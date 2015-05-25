<?php
// $Id$

/**
 * @file
 * IDS KS Item class.
 *
 */

class IdsKsItem extends IdsKsObject {
  public $display_title; // Label of the item to display. Saved for performance.
  public $path; // Only used for untyped subitems (similar to path in the literals).
  public $type;
  public $subitems;
  public $related_items;
  public $root_attributes;
  public $versions_literals;
  public $track_changes;
  public $is_mapping;
  public $is_merged;

  public static function equal($item1, $item2) {
    $are_equal = FALSE;
    if (is_null($item1) && is_null($item2)) {
      $are_equal = TRUE;
    }
    elseif (is_a($item1, 'IdsKsItem') && is_a($item2, 'IdsKsItem')) {
      if (($item1->path == $item2->path) && ($item1->type == $item2->type)) {
        $values1 = $item1->getValues();
        $values2 = $item2->getValues();
        $are_equal = ($values1 == $values2);
      }      
    }
    else {
      $are_equal = FALSE;
    }
    return $are_equal;
  }

  public static function getLatestChangeById($item_id) {
    $latest_id = 0;
    $changes = self::getChildrenIds($item_id, 'track_changes');
    if (!empty($changes)) {
      $latest_id = end($changes);
    }
    return $latest_id;
  }

  /* Get the sources from where the item was updated at some point.
  public function getSourcesFromChanges() {
    $sources = array();
    if (!empty($this->track_changes)) {
      foreach ($this->track_changes as $change) {
        if (isset($change->source)) {
          $sources[] = $change->source;
        }
      }
    }
    $sources = array_unique($sources, SORT_REGULAR);
    return $sources;
  }
  */

  // Get the sources from where the item was updated at some point.
  public function getLatestChangeFromSourceCode($source_code) {
    $found_change = NULL;
    if (!empty($this->track_changes)) {
      foreach (array_reverse($this->track_changes) as $change) {
        if (isset($change->source) && $source_code == $change->source->code) {
          $found_change = $change;
          break;
        }
      }
    }
    return $found_change;
  }

  // Get the sources currently present in the item.
  public function getSourcesFromValues() {
    $sources = array();
    if (isset($this->versions_literals)) {
      foreach ($this->versions_literals as $versions_literals) {
        if (isset($versions_literals->literals)) {
          foreach ($versions_literals->literals as $literal) {
            if (isset($literal->source)) {
              $sources[] = $literal->source;
            }
          }
        }
      }
      $sources = array_unique($sources, SORT_REGULAR);
    }
    return $sources;
  }

  // Checks if an item has some content from a source based on its values, without having to check all the values.
  public function hasContentFromSource($source_id) {
    $found = FALSE;
    if (isset($this->versions_literals)) {
      foreach ($this->versions_literals as $versions_literals) {
        if (isset($versions_literals->literals)) {
          foreach ($versions_literals->literals as $literal) {
            if (isset($literal->source) && ($literal->source->getId() == $source_id)) {
              $found = TRUE;
              break 2;
            }
          }
        }
      }
    }    
    return $found;
  }

  // Get the item type based on the source and/or type passed.
  public function setItemType($source, $feed, $type_name) {
    $type = NULL;
    if ($type_name == 'data') {
      if (isset($feed->default_format)) {
        $format = $feed->default_format;
      }
      if ($format) {
        if (isset($format->type_path) && $format->type_path) {
          if ($versions = $this->retrieveVersionsLiteralPath($format->type_path)) {
            $literals = $versions->getLiteralVersionsSource($source);
            if (!empty($literals)) { // get the first value available from the source.
              $type_name = $literals[0]->value;
            }
            elseif (!empty($versions->literals)) { // get the first value available from any source.
              $type_name = $versions->literals[0]->value;
            }
            else {
              trigger_error(t('The path defined in the file format to retrieve the item\'s type is not valid. Using the feed\'s default type.'));
              $type_name = 'feed';
            }
          }
          else {
            trigger_error(t('The path defined in the file format to retrieve the item\'s type is not valid. Using the feed\'s default type.'));
            $type_name = 'feed';
          }
        }
        else {
          $type_name = 'feed';
        }
      }
    }
    if ($type_name == 'feed') {
      if (isset($feed->default_type)) {
        $type = $feed->default_type;
      }
    }
    if (!isset($type)) { // Was not found and could not be retrieved from the source or feed.
      $type = new IdsKsType();
      $type->name = $type_name;
      $type->load();
    }
    if (isset($type) && !$type->getId()) {
      trigger_error(t('The item type does not exist. Using the default type: @default.', array('@default' => IDSKS_DEFAULT_TYPE_ITEMS)));
      $type->name = IDSKS_DEFAULT_TYPE_ITEMS;
      $type->load();
    }
    if (isset($type) && $type->getId()) {
      $this->type = $type;
    }
    else {
      trigger_error(t('The item type could not be assigned.'));
    }
  }

  // Adds attributes in the root tag.
  public function addRootAttributes($attributes, $source=NULL, $path_prefix = '') {
    if (is_array($attributes)) {
      if (isset($attributes['@attributes'])) {
        $attributes = $attributes['@attributes'];
      }
      foreach ($attributes as $attr_name => $attr_value) {
        $attr_name = idskshub_restore_namespace($attr_name);
        if ($attr_name && $attr_value) {
          if ($path_prefix) {
            $attr_name = '[' . $path_prefix . ']:' . $attr_name;
          }
          if (isset($source->code)) {
            $attr_name = '[' . $source->code . ']:' . $attr_name;
          }
          $new_attribute = IdsKsAttribute::getAttribute($attr_name, $attr_value);
          if (empty($this->root_attributes) || !in_array($new_attribute, $this->root_attributes)) {
            $this->root_attributes[] = $new_attribute;
          }
        }
      }
    }
  }

  // Returns an array with the value of each literal indexed by path and source id.
  public function getValues() {
    $values = array();
    foreach ($this->versions_literals as $versions_literals) {
      foreach ($versions_literals->literals as $literal) {
        $source_id = $literal->source->getId();
        $values[$versions_literals->path][$source_id][] = $literal->value;
      }
    }
    return $values;
  }

  // Returns an array with the value of each literal indexed by source code and path.
  public function getValuesBySource() {
    $values = array();
    foreach ($this->versions_literals as $versions_literals) {
      foreach ($versions_literals->literals as $literal) {
        if (isset($literal->source)) {
          if ($source_code = $literal->source->code) {
            $values[$source_code][$versions_literals->path][] = $literal->value;
          }
        }
      }
    }
    return $values;
  }

  // Function that retrieves the values of those specific fields defined in the item's type (eg: 'title_path' / 'description_path').
  public function getDisplayFieldValue($field, $preferred_lang = IDSKS_DEFAULT_LANGUAGE, $preferred_source_code = '') {
    $values = $this->getDisplayFieldValues($field);
    $field_values = array();
    $value = '';
    if (!empty($values)) {
      foreach ($values as $path => $values_path) {
        if (isset($values_path[$preferred_lang]) && !empty($values_path[$preferred_lang])) {
          if (isset($values_path[$preferred_lang][$preferred_source_code]) && !empty($values_path[$preferred_lang][$preferred_source_code])) {
            $field_values = $values_path[$preferred_lang][$preferred_source_code];
          }
          else {
            $field_values = $values_path[$preferred_lang];
          }
        }
        break;
      }
      if (empty($field_values) && !empty($values)) { // there's no value in the preferred language, we return the first one in any language.
        $field_values = $values;
      }
    }
    if (!empty($field_values)) {
      $flattened = idskshub_flatten_array($field_values);
      foreach ($flattened as $field_value) {
        if ($field_value) { // It's used for strings, so it wouldn't be necessary to test against 0 or FALSE values.
          $value = $field_value;
          break;
        }
      }
    }
    return $value;
  }

  // Function that retrieves the values of fields according to the paths defined in the item's type (eg: 'title').
  // returns $values[source_code][lang_code] = array(....);
  public function getDisplayFieldValues($field, $source_code='') {
    $values = array();
    $field_paths = $field . '_paths';
    if (isset($this->type)) {
      $type = $this->type;
      if (!empty($type->{$field_paths})) {
        $paths = $type->{$field_paths};
        foreach ($paths as $path) {
          $values[$path] = $this->getPathValues($path, $source_code);
        }
      }
    }
    return $values;
  }

  // returns $values[lang_code][source_code] = array(....);
  public function getPathValues($path, $source_code='') {
    $values = array();
    $root_languages = $this->getRootLanguages();
    $versions = $this->retrieveVersionsLiteralPath($path);
    if (!empty($versions) && isset($versions->literals)) {
      foreach ($versions->literals as $literal) {
        $language = '';
        if (isset($literal->source)) {
          if (!$source_code || ($literal->source->code == $source_code)) {
            $source = $literal->source;
            if (isset($literal->attributes)) {
              foreach ($literal->attributes as $attribute) {
                if ($attribute->name == IDSKS_LANGUAGE_ATTRIBUTE) {
                  $language = $attribute->value;
                }
                break;
              }
            }
            if (!$language) {
              if (isset($root_languages[$literal->source->code])) {
                $language = $root_languages[$literal->source->code];
              }
              elseif (isset($source->default_language)) {
                $language = $source->default_language;
              }
              else {
                $language = IDSKS_UNDEFINED_LANGUAGE;
              }
            }
            $values[$language][$source->code][] = $literal->value;
          }
        }
      }
    }
    return $values;
  }

  // If language is set in a root attribute or there is a metadata_languages field we return those values.
  public function getRootLanguages() {
    $metadata_languages = array();
    if ($language_codes = array_keys(idskshub_supported_languages())) {
      $language_codes_regexp = '/(' . implode('|', $language_codes) . ')/';
    }
    else {
      $language_codes_regexp = '/([a-z]){2}/';
    }
    if (isset($this->root_attributes)) {
      foreach ($this->root_attributes as $attribute) {
        if (preg_match('/\[(\w+)\]:'.IDSKS_LANGUAGE_ATTRIBUTE.'/', $attribute->name, $matches)) {
          $source_code = $matches[1];
          if (preg_match($language_codes_regexp, $attribute->value, $matches)) {
            $metadata_languages[$source_code] = $matches[0];
          }
        }
      }
    }
    return $metadata_languages;
  }

  public function getGlobalLanguage($source) {
    $language = FALSE;
    $metadata_languages = idskshub_flatten_array($this->getPathValues(IDSKS_LANGUAGES_FIELD, $source->code));
    if (count($metadata_languages) == 1) {
      $language = $metadata_languages[0];
    }
    else {
      $root_languages = $this->getRootLanguages();
      if (isset($root_languages[$source->code])) {
        $language = $root_languages[$source->code];
      }
      else {
        if (isset($source->default_language)) {
          $language = $source->default_language;
        }
      }
    }
    return $language;
  }

  public function save() {
    if (empty($this->is_mapping)) {
      $this->is_mapping = 0;
    }
    if (empty($this->is_merged)) {
      $this->is_merged = 0;
    }
    parent::save();
  }

  // Save the item with its literals and the track_changes, cleaning up empty literal versions container.
  // Assumes that each referred subitem and related_item has already been saved.
  public function saveItem($save_literals = TRUE, $reindex_parents = TRUE) {
    if (!($this->isEmptyItem())) {
      if (!empty($this->versions_literals)) {
        foreach ($this->versions_literals as $key => $versions_literal) {
          if (!empty($versions_literal->literals)) {
            if ($save_literals) {
              foreach ($versions_literal->literals as $literal) {
                if ($literal) {
                  $literal->save();
                }
              }
            }
            if ($versions_literal) {
              $versions_literal->save();
            }
          }
          else {
            unset($this->versions_literals[$key]);
          }
        }
        $this->versions_literals = array_values(array_filter($this->versions_literals));
      }
      if (isset($this->track_changes)) {
        $track_changes = end($this->track_changes);
        if ($track_changes) { 
          if (!($track_changes->getId())) { // If the most recent changes are not saved, we save it.
            $track_changes->save();
          }
        }
      }
      $this->save();
      if ($reindex_parents) {
        $parents_ids = array();
        $referring_related_items_ids = IdsKsRelatedItem::getParentsIds(array('referred_item' => $this->getId()));
        foreach ($referring_related_items_ids as $referring_related_item_id) {
          $referring_items_ids = IdsKsItem::getParentsIds(array('related_items' => $referring_related_item_id));
          $parents_ids = array_merge($parents_ids, $referring_items_ids);
        }
        IdsKsItem::markToIndex($parents_ids);
      }
    }
    else {
      if ($this->isSaved()) {
        $this->delete();
      }
    }
  }

  public static function markToIndex($item_ids) {
    IdsKsItem::touchMultiple($item_ids);
  }

  // Delete an item based on its item id.
  public static function deleteItem($item_id, $delete_elements = TRUE, $delete_from_parents = TRUE) {
    $item = new IdsKsItem();
    $item->load($item_id);
    if ($item->isSaved()) {
      $item->delete($delete_elements, $delete_from_parents);
      return TRUE;
    }
    else {
      return FALSE;
    }
  }

  // Delete an item with its literals and removes it from its "parents" subitems lists.
  public function delete($delete_elements = TRUE, $delete_from_referring = TRUE) {
    if ($delete_elements) {
      if (!empty($this->versions_literals)) {
        foreach ($this->versions_literals as $versions_literal) {
          if ($versions_literal) {
            IdsKsVersionsLiteral::deleteVersionsLiteral($versions_literal->getId(), FALSE);
          }
        }
      }
      if (!empty($this->track_changes)) {
        foreach ($this->track_changes as $changes) {
          if ($changes) {
            $changes->delete();
          }
        }
      }
      if (!empty($this->related_items)) {
        foreach ($this->related_items as $related_item) {
          if (isset($related_item)) {
            $related_item->delete();
          }
        }
      }
      if (!empty($this->subitems)) {
        foreach ($this->subitems as $subitem) {
          if (isset($subitem) && !$subitem->isTypedItem()) {
            $subitem->delete(TRUE);
          }
        }
      }
    }
    if ($delete_from_referring) { // FALSE when the parent will be deleted anyway (see merge).
      // Delete from referring items via 'related_items'.
      $referring_related_items_ids = IdsKsRelatedItem::getParentsIds(array('referred_item' => $this->getId()));
      foreach ($referring_related_items_ids as $referring_related_item_id) {
        $referring_items_ids = IdsKsItem::getParentsIds(array('related_items' => $referring_related_item_id));
        foreach ($referring_items_ids as $referring_item_id) { // There should be only one.
          $referring_item = new IdsKsItem();
          $referring_item->load($referring_item_id);
          $related_items = array();
          foreach ($referring_item->related_items as $related_item) {
            if ($related_item->referred_item->getId() <> $this->getId()) {
              $related_items[] = $related_item;
            }
            else {
              $related_item->delete();
            }
          }
          $referring_item->related_items = $related_items;
          $referring_item->save();        
        }
      }
      // Delete from referring items via 'subitems'.
      $parent_items_ids = IdsKsItem::getParentsIds(array('subitems' => $this->getId()));
      foreach ($parent_items_ids as $parent_id) {
        $parent = new IdsKsItem();
        $parent->load($parent_id);
        $subitems = array();
        foreach ($parent->subitems as $child) {
          if ($child->getId() <> $this->getId()) {
            $subitems[] = $child;
          }
        }
        $parent->subitems = $subitems;
        $parent->save();
      }
    }
    parent::delete();
  }

/*
  public function removeVersionsLiteralById($versions_id) {
    $new_versions_literals = array();
    foreach ($this->versions_literals as $versions) {
      if ($versions_id != $versions->getId()) {
        $new_versions_literals[] = $versions;
      }
    }
    $this->versions_literals = $new_versions_literals;
    $this->save();
  }
*/

  // Save 'untyped' subitems, which have not been saved in the process of populating the temporary item.
  public function saveSubitems() { 
    if (!empty($this->subitems)) { // it's not null.
      foreach ($this->subitems as $subitem) {
        if (!$subitem->isTypedItem()) {
          $subitem->saveSubitems();
          $subitem->saveItem(TRUE, FALSE);
        }
      }
    }
  }

  // Save links to related items that have not been saved.
  public function saveRelatedItems() { 
    if (!empty($this->related_items)) { // it's not null.
      foreach ($this->related_items as $related_item) {
        $related_item->save();
      }
    }
  }

  // Return TRUE if the item does not have literals.
  public function isEmptyItem() {
    if (!$empty = empty($this->versions_literals)) {
      $empty = TRUE;
      foreach ($this->versions_literals as $versions_literal) {
        if(!empty($versions_literal) && !($versions_literal->isEmptyVersions())) {
          $empty = FALSE;
          break;
        }
      }
    }
    return $empty;
  }

  // Adds atomic values for the *existing* (mapped) paths ids in $values_paths.
  public function addValues($values_paths, $source = NULL) {
    if (empty($source)) {
      $source = idskshub_hub_source();
    }
    foreach ($values_paths as $path => $value) {
      $versions_literals = $this->getVersionsLiteralPath($path);
      $versions_literals->addLiteralXML($value, $source, $path);
    }
  }

  public function addChanges($source, $raw_data='', $hash=0) {
    $track_changes = new IdsKsTrackChanges();
    $track_changes->source = $source;
    $track_changes->timestamp = time();
    if ($raw_data) {
      //$track_changes->raw_data = idskshub_restore_namespace($raw_data);
      $track_changes->raw_data = $raw_data;
      if ($hash) {
        $track_changes->hash = $hash;
      }
      else {
        $track_changes->hash = IdsKsTrackChanges::newHash($raw_data);
      }
    }
    $this->track_changes[] = $track_changes;
  }
  
  // Merge two *saved* items of the same type.
  public static function mergeItems($item1, $item2) {
    debug('Merging items...');
    debug($item1, 'item1');
    debug($item2, 'item2');
    $merged_item_id = 0;
    if ($item1->isSaved() && $item2->isSaved()) {
      if ($item1->getId() <> $item2->getId()) {
        if ($item1->type->getId() == $item2->type->getId()) { // Only merge items if they have the same type.
          if (IdsKsItem::equal($item1, $item2) && !empty($item2->versions_literals)) {
            $item2->versions_literals->delete(FALSE, TRUE);
          }
          elseif (!empty($item2->versions_literals)) {
            foreach ($item2->versions_literals as $versions_literal2) {
              $versions_literal1 = $item1->getVersionsLiteralPath($versions_literal2->path);
              if ($versions_literal1->getId()) { // there are literals with this path in item1.
                foreach ($versions_literal2->literals as $literal2) {
                  if (isset($literal2->source)) {
                    $literals_path_source1 = $versions_literal1->getLiteralVersionsSource($literal2->source);
                    $versions_literal1->addLiteralValue($literal2, $literals_path_source1, 'allow_multiple');
                  }
                }
              } 
              else {
                $item1->versions_literals[] = $versions_literal2;          
              }
            }
          }          
          if ($display_title = $item1->getDisplayFieldValue('title')) {
            $item1->display_title = $display_title;
          }
          $item1->track_changes = is_array($item1->track_changes) ? $item1->track_changes : array();
          $item2->track_changes = is_array($item2->track_changes) ? $item2->track_changes : array();
          $item1->track_changes = array_merge($item1->track_changes, $item2->track_changes);
          foreach ($item2->subitems as $subitem2) {
            if ($subitem2) { 
              if(!$item1->subitemExists($subitem2)) {
                $item1->subitems[] = $subitem2;
              }
              else {
                $subitem2->delete(TRUE, FALSE); // delete its sub-elements but no need to delete from the parent item2, as it will be deleted anyway.
              }
            }
          }
          foreach ($item2->related_items as $related_item2) {
            if (isset($related_item2->referred_item) && $item1->getId() != $related_item2->referred_item->getId() && !$item1->relatedItemExists($related_item2)) {
              $item1->related_items[] = $related_item2;
            }
            else {
              $related_item2->delete();
            }
          }
          $item1->is_merged = 1;
          $item1->saveItem();
          // Change related items pointing to item2 so they now point to item1 (if they werent' already).
          $referring_relitems_item2_ids = IdsKsRelatedItem::getParentsIds(array('referred_item' => $item2->getId()));
          foreach ($referring_relitems_item2_ids as $referring_relitem_item2_id) {
            $relitem_parent_item2 = new IdsKsRelatedItem;
            $relitem_parent_item2->load($referring_relitem_item2_id);
            $referring_items_ids = IdsKsItem::getParentsIds(array('related_items' => $referring_relitem_item2_id));
            $parent_item_id = end($referring_items_ids); // There should be exactly one for each relationship.
            $parent_item = new IdsKsItem;
            $parent_item->load($parent_item_id);
            $relitem_parent_item1 = new IdsKsRelatedItem; // Dummy related item to check if it exists.
            $relitem_parent_item1->source = $relitem_parent_item2->source;
            $relitem_parent_item1->relationship = $relitem_parent_item2->relationship;
            $relitem_parent_item1->referred_item = $item1;
            // If there isn't already a relationship (with the same source and type of rel.) to item1 in item2's parent, we change item2 by item1 and save the existing relationship.
            if (!$parent_item->relatedItemExists($relitem_parent_item1)) {
              $relitem_parent_item2->referred_item = $item1;
              $relitem_parent_item2->save();
            }
          }
          $item2->delete(FALSE, TRUE); // We don't delete the literals, changes, subitems and related items, as they are now part of item1. We do delete it from its parents.
        }
        else {
          trigger_error(t('Items of different types cannot be merged.'));
        }
      }
      else {
        trigger_error(t('It is not possible to merge an item with itself.'));
      }
      $merged_item_id = $item1->getId();
    }
    return $merged_item_id;
  }

  public static function mergeMultipleItemsById($items_ids) {
    debug($items_ids, 'Merging...');
    if (count($items_ids) > 1) {
      $item1_id = array_shift($items_ids);
      $item2_id = array_shift($items_ids);
      $item1 = new IdsKsItem();
      $item1->load($item1_id);
      $item2 = new IdsKsItem();
      $item2->load($item2_id);
      if ($merged_item_id = IdsKsItem::mergeItems($item1, $item2)) {
        array_unshift($items_ids, $merged_item_id);
        return IdsKsItem::mergeMultipleItemsById($items_ids);
      }
      else {
        return array();
      }
    }
    else {
      return $items_ids;
    }
  }

  // Map items.
  public static function mapItems($type_id, $source1_id, $source2_id, $identifiers) {
    $mapped_item = NULL;
    $type = new IdsKsType();
    $type->load($type_id);
    $source1 = new IdsKsSource();
    $source1->load($source1_id);
    $source2 = new IdsKsSource();
    $source2->load($source2_id);
    if ($type->isSaved()) {
      $compound_keys = (count($identifiers[1]) > 1); // we are now using just one key to identify the item if it's not compound.
      $item1_ids = IdsKsItem::retrieveItemIdsByKeys($identifiers[1], $compound_keys, $source1, $type);
      $compound_keys = (count($identifiers[2]) > 1); // we are now using just one key to identify the item if it's not compound.
      $item2_ids = IdsKsItem::retrieveItemIdsByKeys($identifiers[2], $compound_keys, $source2, $type);
      if (empty($item1_ids)) {
        if (empty($item2_ids)) {
          $mapped_item = new IdsKsItem();
          $mapped_item->type = $type;
          $mapped_item->addValues($identifiers[1], $source1);
          $mapped_item->addChanges($source1);
          $mapped_item->addValues($identifiers[2], $source2);
          $mapped_item->addChanges($source2);
        }
        else {
          $mapped_item = new IdsKsItem();
          $mapped_item->load(end($item2_ids));
          $mapped_item->addValues($identifiers[1], $source1);
          $mapped_item->addChanges($source1);
        }
        if ($display_title = $mapped_item->getDisplayFieldValue('title')) {
          $mapped_item->display_title = $display_title;
        }
        else {
            $mapped_item->display_title = "$type->name ($source1->name, $source2->name)";
        }
        $mapped_item->is_mapping = 1;
        $mapped_item->saveItem();
      }
      else {
        if (empty($item2_ids)) {
          $mapped_item = new IdsKsItem();
          $mapped_item->load(end($item1_ids));
          $mapped_item->addValues($identifiers[2], $source2);
          $mapped_item->addChanges($source2);
          if ($display_title = $mapped_item->getDisplayFieldValue('title')) {
            $mapped_item->display_title = $display_title;
          }
          else {
            $mapped_item->display_title = "$type->name ($source1->name, $source2->name)";
          }
          $mapped_item->saveItem();
        }
        else { // both items already exist. we merge them.
          $item1 = new IdsKsItem();
          $item1->load(end($item1_ids));
          $item2 = new IdsKsItem();
          $item2->load(end($item2_ids));
          $item1->is_mapping = 1;
          IdsKsItem::mergeItems($item1, $item2);
          $mapped_item = $item1;
        }
      }
      drupal_set_message(t('The mapping was created.'));
      drupal_set_message(l('Click here to view the new mapping', url('item/'.$mapped_item->getId(), array('absolute' => TRUE))));
    }
    else {
      drupal_set_message(t('Please check that the selected type exist.') . t('The mapping was not created'), 'warning');
    }
    return $mapped_item;
  }

  // Returns an array of item ids that are duplicated in relation to the current item, based on the values of the source's ids.
  public function getDuplicatesBySource($source) {
    $items_ids = array();
    $values_identifiers = array();
    if (!empty($source->identifiers)) {
      foreach ($source->identifiers as $path_key) {
        if (!empty($this->versions_literals)) {
          foreach ($this->versions_literals as $versions_literal) {
            if ($versions_literal->path == $path_key) {
              foreach ($versions_literal->literals as $literal) {
                if ($literal->source == $source) {
                  $values_identifiers[$path_key] = $literal->value;  
                  break; // as it's a key for this source there should be only one.
                }
              }
              break;        
            }
          }
        }
      }
    }
    if (!empty($values_identifiers)) {
      $items_ids = IdsKsItem::retrieveItemIdsByKeys($values_identifiers, $source->compound_key, $source, $this->type);
    }
    return $items_ids;
  }

  // Returns an array of item ids that are duplicated in relation to the current item, based on the values of the type's ids.
  public function getDuplicatesByType() {
    $items_ids = array();
    if (isset($this->type)) {
      $type = $this->type;
      $values_identifiers = array();
      if (!empty($type->identifiers)) {
        foreach ($type->identifiers as $path_key) {
          if (isset($this->versions_literals) && !empty($this->versions_literals)) {
            foreach ($this->versions_literals as $versions_literal) {
              if (isset($versions_literal->literals) && !empty($versions_literal->literals) && ($versions_literal->path == $path_key)) {
                $literal = end($versions_literal->literals); // It is assumed that if two items are the same, the values for the keys from all the sources will be the same.
                if (isset($literal->value)) {
                  $values_identifiers[$path_key] = $literal->value;  
                  break;        
                }
              }
            }
          }
        }
      }
      if (!empty($values_identifiers)) {
        $items_ids = IdsKsItem::retrieveItemIdsByKeys($values_identifiers, $type->compound_key, NULL, $type);
      }
    }
    return $items_ids;
  }


  /* Returns an item based on the values present of the paths passed in $path_values (according to the source and type if present).
  public static function retrieveItemByValues($path_values, $compound_keys = FALSE, $source = NULL, $item_type = NULL) {
    $found_item = NULL;
    $found_item_ids = IdsKsItem::retrieveItemIdsByKeys($path_values, $compound_keys, $source, $item_type);
    if (!empty($found_items_ids)) { // we get the first one that matches the type.
      $item = new IdsKsItem();
      foreach ($found_items_ids as $item_id) {
        $item->load($item_id);
        if (isset($item_type)) {
          if (isset($item->type) && ($item->type == $item_type)) {
            $found_item = $item;
          }
        }
        else {
          $found_item = $item;
        }
        if (!empty($found_item)) {
          break;
        }
      }
    }
    return $found_item;
  }
  */

  // Returns an item based on the values present of the identifiers passed in $path_values (according to the source and type if present).
  public static function retrieveItemIdsByKeys($path_values, $compound_keys = FALSE, $source = NULL, $type = NULL) {
    $literals = array();
    $found_items_ids = array();
    if (isset($type)) {
      $filters['type'] = $type->getId();
    }
    else {
      $filters = array();
    }
    foreach ($path_values as $path => $value) {
      $versions = IdsKsVersionsLiteral::getLiteralsVersions($path, $value, $source);
      $items_ids_path = array();
      foreach ($versions as $versions_literal) {
        $filters['versions_literals'] = $versions_literal->getId(); 
        $items_ids = IdsKsItem::getParentsIds($filters);
        $items_ids_path = array_merge($items_ids_path, $items_ids);
      }
      if (!$compound_keys) {
        $found_items_ids = array_merge($found_items_ids, $items_ids_path);
      }
      elseif (empty($found_items_ids)) {
        $found_items_ids = $items_ids_path;
      } else {
        $found_items_ids = array_intersect($items_ids_path, $found_items_ids);
      }
    }
    return array_unique($found_items_ids);
  }

// Retrieve the versions of the literal based on its path.
  public function retrieveVersionsLiteralPath($path) {
    $found_versions_literal = NULL;
    if (isset($this->versions_literals)) {
      foreach ($this->versions_literals as $versions_literal) {
        if (isset($versions_literal->path) && ($versions_literal->path == $path)) {
          $found_versions_literal = $versions_literal;
          break;
        }
      }
    }
    return $found_versions_literal;
  }

  // If versions exists for this literal's path, they are retrieved, otherwise, a new versions literal is created (this is the difference with retrieveVersionsLiteralPath).
  public function getVersionsLiteralPath($mapped_path) {
    $found_versions_literal = $this->retrieveVersionsLiteralPath($mapped_path);
    if (!$found_versions_literal) {
      if (!isset($this->versions_literals)) { // the array does not exist.
        $this->versions_literals = array();
      }
      $found_versions_literal = new IdsKsVersionsLiteral();
      $found_versions_literal->path = $mapped_path;
      $this->versions_literals[] = $found_versions_literal;
    }
    return $found_versions_literal;
  }

  // Deletes all values in the item for the indicated source.
  // TODO: Change back to protected.
  public function deleteLiteralsSource($source) {
    if (isset($this->versions_literals)) {
      foreach ($this->versions_literals as $key => $versions_literal) {
        $delete_literals_ids = array();
        if (isset($versions_literal->literals)) {
          foreach ($versions_literal->literals as $literal) {
            if (isset($literal->source) && $literal->source == $source) {
              $delete_literals_ids[] = $literal->getId();
            }
          }
          $versions_literal->deleteLiteralsByIds($delete_literals_ids);
        }
        if (empty($versions_literal->literals)) {
          unset($this->versions_literals[$key]);
          $versions_literal->delete(FALSE, FALSE);          
        }
      }
      $this->versions_literals = array_values(array_filter($this->versions_literals));
    }
  }

  // Removes all subitems corresponding to the source from the subitems list of the item.
  protected function removeSubitemsSource($source) {
    if (isset($this->subitems)) {
      foreach ($this->subitems as $key => $subitem) {
        if (!($subitem->isTypedItem())) { // if it's not a typed subitem, it's specific of this item.
          $subitem->deleteLiteralsSource($source);
          $subitem->removeSubitemsSource($source);
          if ($subitem->isEmptyItem()) {
            $subitem->delete();
          }
        }
        unset($this->subitems[$key]);
      }
      //$this->subitems = array_values($this->subitems);
      $this->subitems = array_values(array_filter($this->subitems));
    }
  }

  // Removes all root attirbutes corresponding to the source from the item.
  protected function removeRootAttributesSource($source) {
    if (isset($this->root_attributes)) {
      foreach ($this->root_attributes as $key => $root_attribute) {
        if (preg_match('/^\['.$source->code.'\]:/', $root_attribute->name)) {
          unset($this->root_attributes[$key]);
        }
      }
      $this->subitems = array_values(array_filter($this->subitems));
    }
  }

  // Removes all related items corresponding to the source from the related_items list of the item.
  protected function removeRelatedItemsSource($source) {
    if (isset($this->related_items)) {
      foreach ($this->related_items as $key => $related_item) {
        if (isset($related_item->source) && ($related_item->source == $source)) {
          $related_item->delete();
          unset($this->related_items[$key]);
        }
      }
      $this->related_items = array_values(array_filter($this->related_items));
    }
  }

  protected function subitemExists($new_subitem) {
    $found = FALSE;
    if (isset($this->subitems)) {
      foreach ($this->subitems as $subitem) {
        if (IdsKsItem::equal($new_subitem, $subitem)) {
          $found = TRUE;
          break;
        }
      }
    }
    return $found;
  }

  protected function relatedItemExists($new_related_item) {
    $found = FALSE;
    $related_items_referred_item = isset($new_related_item->referred_item) ? IdsKsRelatedItem::getParentsIds(array('referred_item' => $new_related_item->referred_item->getId())) : array();
    $related_items_item = IdsKsItem::getChildrenIds($this->getId(), 'related_items');
    $candidate_related_items_ids = array_intersect($related_items_item, $related_items_referred_item);
    foreach ($candidate_related_items_ids as $related_item_id) {
      $related_item = new IdsKsRelatedItem();
      $related_item->load($related_item_id);
      if ($found = ($related_item->source == $new_related_item->source) && ($related_item->relationship == $new_related_item->relationship)) {
        break;
      }
    }
    return $found;
  }

  // Populates an item with the values present in the XML passed. Main function used to import items.
  public function populateItemXML($xml_item, $item_path, $source, $format, $recursion = FALSE, $collected_attributes = array()) {
    $versions_literals = array();
    $root_attributes_xml = (array) $xml_item->attributes();
    $root_attributes = array();
    if (!empty($root_attributes_xml)) {
      if (!$recursion) {
        $this->addRootAttributes($root_attributes_xml, $source, $item_path);
      }
      $root_attributes = IdsKsAttribute::getAttributesArray($root_attributes_xml);
      $collected_attributes = array_unique(array_merge($root_attributes, $collected_attributes), SORT_REGULAR);
    }
    foreach ($xml_item as $xml_tag => $xml_value) { 
      $xml_tag = idskshub_restore_namespace($xml_tag);
      $xml_path = $item_path . '/' . $xml_tag;
      if (($xml_value->count() > 0) || ((string) $xml_value)) { // If it's not an empty tag.
        $attributes_xml = (array) $xml_value->attributes();
        $attributes_value = IdsKsAttribute::getAttributesArray($attributes_xml);
        $attributes_value = array_unique(array_merge($attributes_value, $collected_attributes), SORT_REGULAR); // add the parent's attributes that are not in the current element, to use in mappings.
        $new_path_mapped = $xml_path;
        if ($mapping = IdsKsMapping::getMapping($xml_path, $attributes_value, $format, $source, $this->type)) {
          if (isset($mapping->path_destination) && $mapping->path_destination) {
            $new_path_mapped = $mapping->path_destination;
          }
        }
        if (!(isset($mapping->action) && ($mapping->action == 'IGNORE'))) { // Not set to IGNORE.
          if (isset($mapping->action) && ($mapping->action == 'PREPROCESS') && (isset($mapping->path_destination))) {
            $callback = $mapping->path_destination;
            if (is_callable($callback)) {
              $xml_new_value = $callback($xml_value, $source, $xml_path, $collected_attributes);
              if (is_a($xml_new_value, 'SimpleXMLElement')) {
                $xml_new_tag = $xml_new_value->getName();
                $xml_path .= '/' . $xml_new_tag;
                $this->populateItemXML($xml_new_value, $xml_path, $source, $format, TRUE, $collected_attributes);
              }
              else {
                trigger_error(t('Callback function @callback did not return a valid SimpleXMLElement and the element @field_path was ignored.', array('@callback' => $callback, '@field_path' => $xml_path)));
              }
            }
            else {
              trigger_error(t('Callback function @callback could not be invoked and the element @field_path was ignored. Please make sure that the function is defined in idskshub.callback.inc.', array('@callback' => $callback, '@field_path' => $xml_path)));
            }
          }
          elseif (isset($mapping->relationship)) { // Create a new relationship to a related item.
            $is_literal = FALSE;
            if (isset($mapping->path_destination) && $mapping->path_destination) {
              $xml_value->addChild($mapping->path_destination, (string)$xml_value);
            }
            elseif (!$xml_value->count()) { // If it's not a structured value and no mapping field has been defined, we add a default field.
              $xml_value->addChild(IDSKS_DEFAULT_MAPPED_FIELD, (string)$xml_value); //TODO: Check the type's title fields, if they are defined.
            }
            $new_referred_item = new IdsKsItem();
            $new_referred_item->type = $mapping->relationship->item_type;
            $new_referred_item->populateItemXML($xml_value, $xml_path, $source, $format);
            $referred_item_ids = $new_referred_item->getDuplicatesBySource($source); // Look by the source's ids.
            if ($not_found_by_source = empty($referred_item_ids)) { 
              $referred_item_ids = $new_referred_item->getDuplicatesByType(); // Look by type.
            }
            if (empty($referred_item_ids)) { // It's a new item of this type. We save it.
              $referred_item = $new_referred_item;
              $referred_item->saveItem();
              $referred_item->saveSubitems();
              $referred_item->saveRelatedItems();
            }
            else { // The item already exists. We load it, add the new values from this source and save it.
              $referred_item_id = end($referred_item_ids);
              $referred_item = new IdsKsItem();
              $referred_item->load($referred_item_id);              
              if ($not_found_by_source && !$referred_item->hasContentFromSource($source->getId())) { // We only update it if it doesn't already have content from this source.
                $referred_item->updateSourceValues($new_referred_item, $source);
                $referred_item->saveItem();
              }
            }
            $related_item = new IdsKsRelatedItem();
            $related_item->source = $source;
            $related_item->relationship = $mapping->relationship;
            $related_item->referred_item = $referred_item;
            $related_item->path = $xml_path;
            $this->related_items[] = $related_item;
          }
          elseif ($xml_value->count() > 0) { // It's a structured value.
            if (!(isset($mapping->action) && ($mapping->action == 'AS_STRING'))) { // The element should not be flattened.
              $is_literal = FALSE;
              $is_an_array = FALSE;
              if (!$skip_tag = (isset($mapping->action) && ($mapping->action == 'SKIP'))) { // The element should not be skipped.
                // Is it an array or a structured subitem? - Count the number of children with the same tag and attributes.
                $tags_children = array();
                $attributes_children = array();
                foreach($xml_value->children() as $child) {
                  $attributes_child = (array) $child->attributes();
                  if (!in_array($attributes_child, $attributes_children)) {
                    $attributes_children[] = $attributes_child;
                  }
                  $tag_child = $child->getName();
                  if (!in_array($tag_child, $tags_children)) {
                    $tags_children[] = $tag_child;
                  }
                }
                $is_an_array = (count($tags_children) < 2 && count($attributes_children) < 2); // All the children have the same tag and attributes - it is an array.
              }
              if ($skip_tag || $is_an_array) { // it's an array or a "skip tag". we don't add another structure, but add each element to the parent item.
                $this->populateItemXML($xml_value, $xml_path, $source, $format, TRUE, $collected_attributes);
              }
              else { // it is a structured subitem.
                $subitem = new IdsKsItem();
                $subitem->type = IdsKsType::getGenericType();
                $subitem->path = $new_path_mapped;
                $subitem->populateItemXML($xml_value, $xml_path, $source, $format);
                $this->subitems[] = $subitem;
              }
            }
            else { // Flatten value and import as string.
              $is_literal = TRUE;
              $xml_value = $xml_value->asXML();
            }
          }
          else {
            $is_literal = TRUE;
          }
          if ($is_literal) {
            if (!isset($versions_literals[$new_path_mapped])) { // retrieve exising versions literal corresponding to the path (or create it if it doesn't exist).
              $versions_literals[$new_path_mapped] = $this->getVersionsLiteralPath($new_path_mapped);
            }
            if (isset($mapping->action) && ($mapping->action == 'SPLIT')) { // Treat values in a comma-separated string as different values.
              $values_array = preg_split('/\s*[,\;]\s*/', (string)$xml_value);
              foreach ($values_array as $value_lit) { // The literal values are added to the item.
                $versions_literals[$new_path_mapped]->addLiteralXML($value_lit, $source, $xml_path, $attributes_value);
              }
            }
            else { // The literal value is added to the item.
              $versions_literals[$new_path_mapped]->addLiteralXML($xml_value, $source, $xml_path, $attributes_value);
            }
          }
        }
      }
    }
    if (isset($this->type)) {
      if ($display_title = $this->getDisplayFieldValue('title')) {
        $this->display_title = $display_title;
      }
    }
  }

  // This function calls to external semantic augmentation services with the most recent data updated for the item. It is called after each update.
  public function addSemanticData($semantic) {
    $semantic_services = idskshub_get_semantic_services(); // TODO: Get from sources.
    $data = '<title>' . $this->getDisplayFieldValue('title') . '</title>'; // TODO. Add other fields and manage through admin interface. Using the item's full XML triggered some errors in Open Calais.
    $data .= '<description>' . $this->getDisplayFieldValue('description') . '</description>';
    foreach ($semantic as $service_code => $checked) {
      if ($checked) {
        $source = new IdsKsSource();
        $source->code = $service_code;
        $source->load();
        if ($source->getId() && $source->is_semantic) {
          $semantic_entities = idskshub_semantic_services_call($service_code, $data);
          foreach ($semantic_entities as $path_entity_name => $entities) {
            $path_mapped = IdsKsMapping::getMappingDestination($path_entity_name);
            $versions_literal = $this->getVersionsLiteralPath($path_mapped);
            foreach ($entities as $entity) {
              $new_literal = new IdsKsLiteral();
              $new_literal->source = $source;
              $new_literal->value = $entity; 
              $new_literal->original_path = $path_entity_name; 
              if (!($versions_literal->inLiterals($new_literal))) {
                $versions_literal->literals[] = $new_literal;
              }
            }
          }
        }
      }
    }
  }

  // Indicates if the item is "typed" (corresponds to one of the main types defined in the hub).
  public function isTypedItem() {
    return ($this->type->name != IDSKS_GENERIC_TYPE_NAME);
  }

  // Updates an existing item with the values of the temporary item $new_item. *Does not save the item*.
  public function updateSourceValues($new_item, $source, $update_mode = 'incremental', $allow_multiple = 'allow_multiple') { // updates values in $this with the ones in $new_item.
    $this->track_changes[] = $new_item->track_changes[0];
    // Literals
    if (($update_mode == 'replace') && !empty($new_item->versions_literals)) {
      $this->deleteLiteralsSource($source);
      if (empty($this->versions_literals)) {
        $this->versions_literals = $new_item->versions_literals;
      }
      else {
        $this->versions_literals = array_merge(array_filter($this->versions_literals), $new_item->versions_literals);
      }
    }
    else {
      // Add literals from the new_item's source (without duplicated values).
      if (!empty($new_item->versions_literals)) {
        foreach ($new_item->versions_literals as $new_versions_literal) {
          $versions_literal = $this->getVersionsLiteralPath($new_versions_literal->path);
          $literals_path_source = $versions_literal->getLiteralVersionsSource($source);
          if (isset($new_versions_literal->literals)) { // Not necessary to filter by source because $new_item just has values from this source.
            foreach ($new_versions_literal->literals as $new_literal) {
              $versions_literal->addLiteralValue($new_literal, $literals_path_source, $allow_multiple);
            }
          }
        }
      }
    }
    // Subitems and attibutes
    if ($update_mode == 'replace') {
      $this->removeSubitemsSource($source);
      $this->removeRelatedItemsSource($source);
      $this->removeRootAttributesSource($source);
    }
    if (!empty($new_item->subitems)) {
      foreach ($new_item->subitems as $new_subitem) {
        if (!($this->subitemExists($new_subitem)) && !($new_subitem->isEmptyItem())) {
          $new_subitem->saveItem(TRUE, FALSE);
          $this->subitems[] = $new_subitem;
        }
      }
    }
    if (!empty($new_item->related_items)) {
      foreach ($new_item->related_items as $new_related_item) {
        if (($this->getId() <> $new_related_item->referred_item->getId()) && !$this->relatedItemExists($new_related_item)) {
          $new_related_item->save();
          $this->related_items[] = $new_related_item;
        }
      }
    }
    if (!empty($new_item->root_attributes)) {
      foreach ($new_item->root_attributes as $new_attribute) {
        if (!in_array($new_attribute, $this->root_attributes)) {
          $this->root_attributes[] = $new_attribute;
        }
      }
    }
    if (empty($this->display_title) && !empty($new_item->display_title)) { // TODO. Assign based on source's "priorities".
      $this->display_title = $new_item->display_title;
    }
  }

} // class IdsKsItem
