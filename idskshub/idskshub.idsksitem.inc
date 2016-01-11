<?php
// $Id$

/**
* @file
* IDS KS Item class.
*
*/

class IdsKsItem extends IdsKsObject {
  public $display_title; // Label of the item to display. Saved for performance.
  public $is_mapping;
  public $is_merged;
  public $path; // Only used for untyped subitems (similar to path in the literals).
  public $type;
  public $subitems;
  public $related_items;
  public $root_attributes;
  public $versions_literals;
  public $track_changes;

  public static function equal($item1, $item2) {
    $are_equal = FALSE;
    if (is_null($item1) && is_null($item2)) {
      $are_equal = TRUE;
    }
    elseif (is_a($item1, 'IdsKsItem') && is_a($item2, 'IdsKsItem')) {
      if (($item1->get('path') == $item2->get('path')) && ($item1->get('type')->getId() == $item2->get('type')->getId())) {
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

  public function getLatestChangeFromSourceCode($source_code) {
    $found_change = NULL;
    if ($this->get('track_changes')) {
      foreach (array_reverse($this->get('track_changes')) as $change) {
        if ($change->get('source') && $source_code == $change->get('source')->get('code')) {
          $found_change = $change;
          break;
        }
      }
    }
    return $found_change;
  }

  public function getLatestSourceCode() {
    $code = '';
    if ($this->get('track_changes')) {
      $latest_change = end($this->get('track_changes'));
      if ($latest_change->get('source')) {
        $code = $latest_change->get('source')->get('code');
      }
    }
    if (!$code) {
      if ($sources = $this->getSourcesFromValues()) {
        $latest_source = end($sources);
        if ($latest_source->get('code')) {
          $code = $latest_source->get('code');
        }
      }
    }
    return $code;
  }

  // Get the sources currently present in the item.
  public function getSourcesFromValues() {
    $sources = array();
    if ($this->get('versions_literals')) {
      foreach ($this->get('versions_literals') as $versions_literals) {
        if ($versions_literals && $versions_literals->get('literals')) {
          foreach ($versions_literals->get('literals') as $literal) {
            if ($literal && $literal->get('source')) {
              $sources[] = $literal->get('source');
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
    if ($this->get('versions_literals')) {
      foreach ($this->get('versions_literals') as $versions_literals) {
        if ($versions_literals->get('literals')) {
          foreach ($versions_literals->get('literals') as $literal) {
            if ($literal && $literal->get('source') && $literal->get('source')->getId() == $source_id) {
              $found = TRUE;
              break 2;
            }
          }
        }
      }
    }    
    return $found;
  }

  public function isExternal() {
    $is_external = TRUE;
    if ($this->get('versions_literals')) {
      foreach ($this->get('versions_literals') as $versions_literals) {
        if ($versions_literals && $versions_literals->get('literals')) {
          foreach ($versions_literals->get('literals') as $literal) {
            if ($literal && $literal->get('source') && !$literal->get('source')->get('is_external')) {
              $is_external = FALSE;
              break 2;
            }
          }
        }
      }
    } 
    return $is_external;
  }

  public function getExternalUriFromStructure($uri_structure, $source) {
    $uri = $uri_structure;
    preg_match_all('/%(?!%).+?%/', $uri_structure, $matches);
    $replace_values = array();
    if ($meta_fields = $matches[0]) {
      foreach($meta_fields as $meta_field) {
        $field_name = trim($meta_field, '%');
        if ($literals = idskshub_flatten_array($this->getPathValues($field_name , $source->get('code')))) {
          $value = $literals[0]; // We use the first one found if there are more than one.
        }
        else {
          $value = '';
        }
        $replace_values['/'.$meta_field.'/'] = $value;
      }
      $uri = preg_replace(array_keys($replace_values), $replace_values, $uri_structure);
    }
    return $uri;
  }

  public function updateExternalRelationships($force_update = FALSE) {
    $sources = $this->getSourcesFromValues();
    foreach ($sources as $source) {
      $sources_ids[] = $source->getId();
    }
    $external_mappings_ids = IdsKsExternalMappings::getIds(array('source_origin' => $sources_ids));
    foreach ($external_mappings_ids as $external_mapping_id) {
      $external_mapping = new IdsKsExternalMappings;
      $external_mapping->load($external_mapping_id);
      $relationship_source = $external_mapping->get('relationship_source');
      $update = TRUE;
      if (!$force_update && $latest_change_source = $this->getLatestChangeFromSourceCode($relationship_source->get('code'))) {
        $update = ($latest_change_source->get('timestamp') + $relationship_source->get('lifespan')*24*60*60 < time());
      }
      if ($update) {
        $this->updateExternalRelationshipsMapping($external_mapping);
        $external_mapping->set('applied', time());
        $external_mapping->save();
      }
    }
  }

  public function updateExternalRelationshipsMapping($external_mapping) {
    $this->addChanges($external_mapping->get('relationship_source'));
    $new_rel_items = FALSE;
    $m = new IdsKsExternalMappings;
    $m->set('source_origin', $external_mapping->get('source_origin'));
    if ($external_mappings = $m->getObjectsByProperties()) {
      // Current item's uri.
      $e = new IdsKsExternalUris;
      $e->set('type', $this->get('type'));
      $e->set('source', $external_mapping->get('source_origin'));
      if ($external_uri_origin = $e->getSingleObjectByProperties()) {
        if ($item_uri = $this->getExternalUriFromStructure($external_uri_origin->get('uris_structure'), $external_mapping->get('source_origin'))) {
          // Get external related items.
          $e = new IdsKsExternalUris;
          $e->set('type', $external_mapping->get('relationship')->get('item_type'));
          $e->set('source', $external_mapping->get('source_destination'));
          if ($external_uri_destination = $e->getSingleObjectByProperties()) {
            $current_item_related_items_ids = IdsKsItem::getChildrenIds($this->getId(), 'related_items');
            $related_items_source_ids = array();
            $mapped_items_uris = idskshub_sparql_retrieve($external_mapping->get('mappings_url'), $item_uri, $external_mapping->get('external_property'));
            foreach ($mapped_items_uris as $mapped_item_uri) {
              if ($mapped_item_uri['object'] <> $item_uri) { // the SPARQL query returns the original item, too. TODO: Add $external_uri_destination->matches($mapped_item_uri) for an extra check.
                $field_values = $external_uri_destination->getValuesFromUri($mapped_item_uri['object']);
                // Item exists in the hub (with values matching those in the uri).
                if ($existing_items_ids = IdsKsItem::retrieveItemIdsByKeys($field_values, TRUE, $external_mapping->get('source_destination'), $external_mapping->get('relationship')->get('item_type'))) {
                  foreach ($existing_items_ids as $existing_item_id) { // In general, it should be only one.
                    $rel_items_existing_item_ids = IdsKsRelatedItem::getIds(array('source' => $external_mapping->get('relationship_source')->getId(), 'referred_item' => $existing_item_id));
                    $rel_items_same_rel_existing_item_ids = IdsKsRelatedItem::getIds(array('source' => $external_mapping->get('relationship_source')->getId(), 'relationship' => $external_mapping->get('relationship')->getId(), 'referred_item' => $existing_item_id));
                    $existing_related_items_ids = array_intersect($rel_items_existing_item_ids, $current_item_related_items_ids);
                    $existing_same_rel_related_items_ids = array_intersect($rel_items_same_rel_existing_item_ids, $current_item_related_items_ids);
                    // If the existing item was not related with the same relationship, we create the new relationship.
                    // If the item was related by another relationship from the same source, we delete the previous relationship first.
                    if (empty($existing_related_items_ids) || $existing_related_items_ids <> $existing_same_rel_related_items_ids) {
                      $this->deleteRelatedItemsByIds($existing_related_items_ids);
                      $new_rel_items = TRUE;
                      $referred_item = new IdsKsItem;
                      $referred_item->load($existing_item_id);
                      $related_item = new IdsKsRelatedItem;
                      $related_item->set('referred_item', $referred_item);
                      $related_item->set('source', $external_mapping->get('relationship_source'));
                      $related_item->set('relationship', $external_mapping->get('relationship'));
                      $related_item->save();
                      $this->add('related_items', $related_item);
                      $related_item->createInverse($this);
                      $related_items_source_ids[] = $related_item->getId();
                    }
                    else {
                      $related_items_source_ids = array_merge($related_items_source_ids, $existing_related_items_ids);
                    }
                  }
                }
                else {
                  // It's a new item and relationship.
                  $new_rel_items = TRUE;
                  $referred_item = new IdsKsItem;
                  $referred_item->set('type', $external_mapping->get('relationship')->get('item_type'));
                  foreach ($field_values as $field => $value) {
                    $versions_literal = new IdsKsVersionsLiteral();
                    $versions_literal->set('path', $field);
                    $versions_literal->addLiteralXML($value, $external_mapping->get('source_destination'), $field);
                    $referred_item->add('versions_literals', $versions_literal);
                  }
                  $referred_item->addChanges($external_mapping->get('source_destination'));
                  $referred_item->saveItem();
                  $related_item = new IdsKsRelatedItem;
                  $related_item->set('referred_item', $referred_item);
                  $related_item->set('source', $external_mapping->get('relationship_source'));
                  $related_item->set('relationship', $external_mapping->get('relationship'));
                  $related_item->save();
                  $this->add('related_items', $related_item);
                  $related_item->createInverse($this);
                  $related_items_source_ids[] = $related_item->getId();
                }
              }
            } // foreach
            if ($new_rel_items) {
              $this->saveItem();
            }
            // Delete existing relationships by the source that are no longer present in the response from the SPARQL endpoint.
            $delete_old_related_items_source = array();
            $sources_rel_items_current_item = IdsKsRelatedItem::getFieldValues('source', $current_item_related_items_ids);
            foreach ($sources_rel_items_current_item as $rel_item_id => $sources_ids) {
              if (in_array($external_mapping->get('relationship_source')->getId(), $sources_ids)) { // It's a related item by the relationship source.
                if (!in_array($rel_item_id, $related_items_source_ids)) { // The related item is no longer valid according to the external source.
                  $delete_old_related_items_source[] = $rel_item_id;
                }
              }
            }
            if ($delete_old_related_items_source) {
              $this->deleteRelatedItemsByIds($delete_old_related_items_source);
            }
          }
        }
      }
    }
  }

  public function updateExternalValues($force_update = FALSE) {
    $sources = $this->getSourcesFromValues();
    foreach ($sources as $source) {
      if ($source->get('is_external')) {
        $update = TRUE;
        if (!$force_update && $latest_change_source = $this->getLatestChangeFromSourceCode($source->get('code'))) {
          $update = ($latest_change_source->get('timestamp') + $source->get('lifespan')*24*60*60 < time());
        }
        if ($update) {
          $this->updateExternalValuesSource($source);
        }
      }
    }
  }

  public function updateExternalValuesSource($source) {
    $updated = FALSE;
    if ($source->get('external_properties') && $source->get('endpoint_url')) {
      $current_values = $this->getValuesBySource();
      if (isset($current_values[$source->get('code')]['uri'])) {
        $uri = end($current_values[$source->get('code')]['uri']);
        $values_paths = array();
        foreach ($source->get('external_properties') as $property) {
          if ($values = idskshub_sparql_retrieve($source->get('endpoint_url'), $uri, $property)) {
            $versions_literals = $this->getVersionsLiteralPath($property);
            $versions_literals->deleteLiteralsBySourceId($source->getId());
            foreach ($values as $value) {
              if (isset($value['object'])) {
                $attributes = array();
                foreach ($value as $key => $val) {
                  $array_key = explode(' ', $key);
                  if (isset($array_key[1])) { // it's an attribute
                    $attr_name = $array_key[1];
                    $attr_value = $val;
                    $attr = IdsKsAttribute::getAttribute($attr_name, $attr_value);
                    $attributes[] = $attr;
                  }
                }
                $updated = TRUE;
                $versions_literals->addLiteralXML($value['object'], $source, $property, $attributes);
              }
            }
          }
        }
        if ($updated) {
          $this->addChanges($source);
          $this->setDisplayTitle();
          $this->saveItem();
        }
      }
    } 
  }

  // Get the item type based on the source and/or type passed.
  public function setItemType($source, $feed, $type_name) {
    $type = NULL;
    if ($type_name == 'data') {
      if ($feed->get('default_format')) {
        $format = $feed->get('default_format');
      }
      if ($format) {
        if ($format->get('type_path')) {
          if ($versions = $this->retrieveVersionsLiteralPath($format->get('type_path'))) {
            $literals = $versions->getLiteralVersionsSource($source);
            if (!empty($literals)) { // get the first value available from the source.
              $type_name = $literals[0]->get('value');
            }
            elseif ($versions->get('literals')) { // get the first value available from any source.
              $first = array_shift($versions->get('literals'));
              $type_name = $first->get('value');
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
      if ($feed->get('default_type')) {
        $type = $feed->get('default_type');
      }
    }
    if (!isset($type)) { // Was not found and could not be retrieved from the source or feed.
      $type = new IdsKsType();
      $type->set('name', $type_name);
      $type->load();
    }
    if (isset($type) && !$type->getId()) {
      trigger_error(t('The item type does not exist. Using the default type: @default.', array('@default' => IDSKS_DEFAULT_TYPE_ITEMS)));
      $type->set('name', IDSKS_DEFAULT_TYPE_ITEMS);
      $type->load();
    }
    if (isset($type) && $type->getId()) {
      $this->set('type', $type);
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
          if ($source->get('code')) {
            $attr_name = '[' . $source->get('code') . ']:' . $attr_name;
          }
          $new_attribute = IdsKsAttribute::getAttribute($attr_name, $attr_value);
          if (!$this->get('root_attributes') || array_udiff($this->get('root_attributes'), array($new_attribute), 'idskshub_diff')) {
            $this->add('root_attributes', $new_attribute);
          }
        }
      }
    }
  }

  // Returns an array with the value of each literal indexed by path and source id.
  public function getValues() {
    $values = array();
    foreach ($this->get('versions_literals') as $versions_literals) {
      if ($versions_literals) {
        foreach ($versions_literals->get('literals') as $literal) {
          if ($literal) {
            $source_id = $literal->get('source')->getId();
            $values[$versions_literals->get('path')][$source_id][] = $literal->get('value');
          }
        }
      }
    }
    return $values;
  }

  // Returns an array with the value of each literal indexed by source code and path.
  public function getValuesBySource() {
    $values = array();
    foreach ($this->get('versions_literals') as $versions_literals) {
      if ($versions_literals) {
        foreach ($versions_literals->get('literals') as $literal) {
          if ($literal && $literal->get('source')) {
            if ($source_code = $literal->get('source')->get('code')) {
              $values[$source_code][$versions_literals->get('path')][] = $literal->get('value');
            }
          }
        }
      }
    }
    return $values;
  }

  // Returns an array with the literals indexed by source code and versions literals id.
  public function getLiteralsBySource() {
    $values = array();
    foreach ($this->get('versions_literals') as $versions_literals) {
      if ($versions_literals) {
        foreach ($versions_literals->get('literals') as $literal) {
          if ($literal && $literal->get('source')) {
            if ($source_code = $literal->get('source')->get('code')) {
              $values[$source_code][$versions_literals->getId()][] = $literal;
            }
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
    if ($this->get('type')) {
      $type = $this->get('type');
      if ($type->get($field_paths)) {
        $paths = $type->get($field_paths);
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
    if (!empty($versions) && $versions->get('literals')) {
      foreach ($versions->get('literals') as $literal) {
        $language = '';
        if ($literal && $literal->get('source')) {
          if (!$source_code || ($literal->get('source')->get('code') == $source_code)) {
            $source = $literal->get('source');
            if ($literal->get('attributes')) {
              foreach ($literal->get('attributes') as $attribute) {
                if ($attribute->get('name') == IDSKS_LANGUAGE_ATTRIBUTE) {
                  $language = $attribute->get('value');
                  break;
                }
              }
            }
            if (!$language) {
              if (isset($root_languages[$literal->get('source')->get('code')])) {
                $language = $root_languages[$literal->get('source')->get('code')];
              }
              elseif ($source->get('default_language')) {
                $language = $source->get('default_language');
              }
              else {
                $language = IDSKS_UNDEFINED_LANGUAGE;
              }
            }
            $values[$language][$source->get('code')][] = $literal->get('value');
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
    if ($this->get('root_attributes')) {
      foreach ($this->get('root_attributes') as $attribute) {
        if ($attribute && $attribute->get('name') && preg_match('/\[(\w+)\]:'.IDSKS_LANGUAGE_ATTRIBUTE.'/', $attribute->get('name'), $matches)) {
          $source_code = $matches[1];
          if (preg_match($language_codes_regexp, $attribute->get('value'), $matches)) {
            $metadata_languages[$source_code] = $matches[0];
          }
        }
      }
    }
    return $metadata_languages;
  }

  // Returns root attributes indexed by source.
  public function getRootAttributesBySource() {
    $root_attributes = array();
    if ($this->get('root_attributes')) {
      foreach ($this->get('root_attributes') as $attribute) {
        if ($attribute && $attribute->get('name') && preg_match('/\[(\w+)\]:/', $attribute->get('name'), $matches)) {
          $source_code = $matches[1];
          $root_attributes[$source_code][] = $attribute;
        }
      }
    }
    return $root_attributes;
  }

  public function setDisplayTitle() {
    if (!$display_title = $this->getDisplayFieldValue('title')) {
      if ($this->get('type') && $this->get('type')->get('name')) {
        $display_title = $this->get('type')->get('name');
      }
      if ($this->getId()) {
        $display_title .= ' ' . $this->getId();
      }
    }
    $this->set('display_title', html_entity_decode($display_title, ENT_COMPAT, "UTF-8"));
  }

  public function getGlobalLanguage($source) {
    $language = FALSE;
    $metadata_languages = idskshub_flatten_array($this->getPathValues(IDSKS_LANGUAGES_FIELD, $source->get('code')));
    if (count($metadata_languages) == 1) {
      $language = $metadata_languages[0];
    }
    else {
      $root_languages = $this->getRootLanguages();
      if (isset($root_languages[$source->get('code')])) {
        $language = $root_languages[$source->get('code')];
      }
      else {
        if ($source->get('default_language')) {
          $language = $source->get('default_language');
        }
      }
    }
    return $language;
  }

  public function save() {
    if (!$this->get('is_mapping')) {
      $this->set('is_mapping', 0);
    }
    if (!$this->get('is_merged')) {
      $this->set('is_merged', 0);
    }
    parent::save();
  }

  public function getRelItemsIds() {
    $rel_items_ids = array();
    foreach ($this->get('related_items') as $rel_item) {
      if ($rel_item && $rel_item->get('referred_item') && $rel_item->get('source') && $rel_item->get('relationship') && ($source_code = $rel_item->get('source')->get('code')) && ($relationship_code = $rel_item->get('relationship')->get('code'))) {
        $rel_items_ids[$relationship_code][$source_code][] = $rel_item->get('referred_item')->getId();
      }
    }
    return $rel_items_ids;
  }

  public function getRelItemsTitles() {
    $rel_items_ids = array();
    foreach ($this->get('related_items') as $rel_item) {
      if ($rel_item->get('referred_item') && $rel_item->get('source') && $rel_item->get('relationship') && ($source_code = $rel_item->get('source')->get('code')) && ($relationship_code = $rel_item->get('relationship')->get('code')) && ($display_title = $rel_item->get('referred_item')->get('display_title'))) {
        $rel_items_ids[$relationship_code][$source_code][] = $display_title;
      }
    }
    return $rel_items_ids;
  }

  // Indicates if the item is a top-level element in all hierarchies of items of its type.
  public function isTopLevel($source_code = '') {
    $top_level = TRUE;
    if ($this->get('type') && ($type = $this->get('type')->get('name'))) {
      if (!defined('IDSKS_DEFAULT_PARENT_REL_CATEGORY')) { // For backwards compatibility - it can be deleted after updating the settings in the live server.
        $parent_rel_code = 'parent_' . strtolower($type);
      }
      else {
        $parent_rel_code = IDSKS_DEFAULT_PARENT_REL_CATEGORY;
      }
      $rel_items = $this->getRelItemsIds();
      if ($source_code) { // is top-level in this source's hierarchy
        $top_level = !isset($rel_items[$parent_rel_code][$source_code]);
      }
      else { // in all hierarchies
        $top_level = !isset($rel_items[$parent_rel_code]);
      }
    }
    return $top_level;
  }

  // Save the item with its literals and the track_changes, cleaning up empty literal versions container.
  // Assumes that each referred subitem and related_item has already been saved.
  public function saveItem($save_literals = TRUE, $reindex_parents = TRUE) {
    if (!$this->isEmptyItem()) {
      // Clean versions_literals and save values if indicated.
      if ($this->get('versions_literals')) {
        $versions_literals = $this->get('versions_literals');
        foreach ($versions_literals as $key => $versions_literal) {
          if ($versions_literal && $versions_literal->get('literals')) {
            if ($save_literals) {
              foreach ($versions_literal->get('literals') as $literal) {
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
            unset($versions_literals[$key]);
          }
        }
        $this->set('versions_literals', array_values(array_filter($versions_literals)));
      }
      // Clean related_items in case there was an error when saving previously.
      if ($related_items = $this->get('related_items')) {
        foreach ($related_items as $key => $related_item) {
          if (!$related_item) {
            unset($related_items[$key]);
          }
        }
        $this->set('related_items', array_values(array_filter($related_items)));
      }
      // Clean subitems in case there was an error when saving previously.
      if ($subitems = $this->get('subitems')) {
        foreach ($subitems as $key => $subitem) {
          if (!$subitem) {
            unset($subitems[$key]);
          }
        }
        $this->set('subitems', array_values(array_filter($subitems)));
      }
      if ($this->get('track_changes') && ($track_changes = end($this->get('track_changes'))) && !$track_changes->isSaved()) { // If the most recent changes are not saved, we save it.
        $track_changes->save();
      }
      $this->save();
      if ($reindex_parents) {
        $this->reindexParents();
      }
    }
    else {
      if ($this->isSaved()) {
        $this->delete();
      }
    }
  }

  public function reindexParents() {
    $parents_ids = array();
    $referring_related_items_ids = IdsKsRelatedItem::getIds(array('referred_item' => $this->getId()));
    foreach ($referring_related_items_ids as $referring_related_item_id) {
      $referring_items_ids = IdsKsItem::getIds(array('related_items' => $referring_related_item_id));
      $parents_ids = array_merge($parents_ids, $referring_items_ids);
    }
    IdsKsItem::markToIndex($parents_ids);
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
      if ($this->get('versions_literals')) {
        foreach ($this->get('versions_literals') as $versions_literal) {
          if ($versions_literal) {
            IdsKsVersionsLiteral::deleteVersionsLiteral($versions_literal->getId(), FALSE);
          }
        }
      }
      if ($this->get('track_changes')) {
        foreach ($this->get('track_changes') as $changes) {
          if ($changes) {
            $changes->delete();
          }
        }
      }
      if ($this->get('related_items')) {
        foreach ($this->get('related_items') as $related_item) {
          if ($related_item) {
            $related_item->delete();
          }
        }
      }
      if ($this->get('subitems')) {
        foreach ($this->get('subitems') as $subitem) {
          if ($subitem && !$subitem->isTypedItem()) {
            $subitem->delete(TRUE);
          }
        }
      }
    }
    if ($delete_from_referring) { // FALSE when the parent will be deleted anyway (see merge).
      // Delete from referring items via 'related_items'.
      $referring_related_items_ids = IdsKsRelatedItem::getIds(array('referred_item' => $this->getId()));
      foreach ($referring_related_items_ids as $referring_related_item_id) {
        $referring_items_ids = IdsKsItem::getIds(array('related_items' => $referring_related_item_id));
        foreach ($referring_items_ids as $referring_item_id) { // There should be only one.
          $referring_item = new IdsKsItem();
          $referring_item->load($referring_item_id);
          $related_items = array();
          foreach ($referring_item->get('related_items') as $related_item) {
            if ($related_item) {
              if ($related_item->get('referred_item') && $related_item->get('referred_item')->getId() <> $this->getId()) {
                $related_items[] = $related_item;
              }
              else {
                $related_item->delete();
              }
            }
          }
          $referring_item->set('related_items', $related_items);
          $referring_item->save();        
        }
      }
      // Delete from referring items via 'subitems'.
      $parent_items_ids = IdsKsItem::getIds(array('subitems' => $this->getId()));
      foreach ($parent_items_ids as $parent_id) {
        $parent = new IdsKsItem();
        $parent->load($parent_id);
        $subitems = array();
        foreach ($parent->get('subitems') as $child) {
          if ($child && $child->getId() <> $this->getId()) {
            $subitems[] = $child;
          }
        }
        $parent->set('subitems', $subitems);
        $parent->save();
      }
    }
    parent::delete();
  }

  // Save 'untyped' subitems, which have not been saved in the process of populating the temporary item.
  public function saveSubitems() { 
    if ($this->get('subitems')) {
      foreach ($this->get('subitems') as $subitem) {
        if (is_object($subitem)) {
          if (!$subitem->isTypedItem()) {
            $subitem->saveSubitems();
            $subitem->saveItem(TRUE, FALSE);
          }
        }
      }
    }
  }

  // Save links to related items that have not been saved.
  public function saveRelatedItems() { 
    if ($this->get('related_items')) {
      foreach ($this->get('related_items') as $related_item) {
        if (is_object($related_item)) { 
          $related_item->save();
        }
      }
    }
  }

  // Create related items "inverse" to the item's related items.
  public function createInverseRelatedItems() { 
    if ($this->get('related_items')) {
      foreach ($this->get('related_items') as $related_item) {
        if (is_object($related_item)) { 
          $related_item->createInverse($this);
        }
      }
    }
  }

  public function deleteRelatedItemsByIds($related_items_ids=array()) {
    if ($related_items_ids) {
      $related_items = array();
      foreach ($this->get('related_items') as $related_item) {
        if ($related_item) {
          if (!in_array($related_item->getId(), $related_items_ids)) {
            $related_items[] = $related_item;
          }
          else {
            $referred_item = $related_item->get('referred_item');
            $related_item->delete();
            // If there are inverse relationship we delete those, too.
            if ($inverse_related_items = IdsKsRelatedItem::getIds(array('referred_item'=>$this->getId()))) {
              $items_ids = IdsKsItem::getIds(array('related_items' => $inverse_related_items));
              if (in_array($referred_item->getId(), $items_ids)) { // If the referred item also points to the current item, we delete that relationship, too.
                $referred_item->deleteRelatedItemsByIds($inverse_related_items);
              }
            }    
          }
        }
      }
      $this->set('related_items', $related_items);
      $this->save();
    }
  }

  // Return TRUE if the item does not have literals.
  public function isEmptyItem() {
    if ($empty = !$this->get('versions_literals')) {
      $empty = TRUE;
      foreach ($this->get('versions_literals') as $versions_literal) {
        if($versions_literal && !($versions_literal->isEmptyVersions())) {
          $empty = FALSE;
          break;
        }
      }
    }
    return $empty;
  }

  // Adds atomic values for the *existing* (mapped) paths ids in $values_paths. TODO: Add attributes.
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
    $track_changes = new IdsKsTrackChanges;
    $track_changes->set('source', $source);
    $track_changes->set('timestamp', time());
    if ($raw_data) {
      $track_changes->set('raw_data', $raw_data);
      if ($hash) {
        $track_changes->set('hash', $hash);
      }
      else {
        $track_changes->set('hash', IdsKsTrackChanges::newHash($raw_data));
      }
    }
    $this->add('track_changes', $track_changes);
  }

  // Splits a merged item, generating one item for each source.
  public function splitBySources() {
    $semantic_codes = array_keys(idskshub_get_semantic_services());
    // Get "same as" relationship based on current item's type.
    $same_as_rel = new IdsKsRelationship;
    //----- Leaving this temporarily for backward compatibility, until the relationships are updated in the live server ---------------
    if (!defined('IDSKS_DEFAULT_MAPPING_REL_CATEGORY')) {
      $rel_code = 'same_as_' . strtolower($this->get('type')->get('name'));
    }
    //---------------------------------------------------------------------------------------------------------------------------------
    else {
      if ($this->get('type')->get('is_category')) {
        $rel_code = IDSKS_DEFAULT_MAPPING_REL_CATEGORY;
      }
      else {
        $rel_code = IDSKS_DEFAULT_MAPPING_REL_ASSET;
      }
    }
    if ($rels_ids = IdsKsRelationship::getIds(array('code' => $rel_code, 'item_type' => $this->get('type')->getId()))) {
    // rel. code and item type should be a key, so there should be only one.
      $same_as_rel->load($rels_ids[0]);
    }
    // Get source corresponding to the OKHub;
    $okhub_source = new IdsKsSource;
    $okhub_source->set('code', IDSKS_OKHUB_SOURCE_CODE);
    $okhub_source->load();
    $vals = $this->getLiteralsBySource();
    $root_attrs = $this->getRootAttributesBySource();
    $new_vers_literals = array();
    $new_items = array();
    array_multisort(array_map('count', $vals), SORT_DESC, $vals);
    $first_source_code = key($vals);
    $first_source_values = array_shift($vals);
    if (count(array_keys($vals)) > 0) { // There are literals' values for more than one source
      foreach ($vals as $source_code => $vers_lits) {
        if (!in_array($source_code, $semantic_codes) && $source_code != IDSKS_OKHUB_SOURCE_CODE) { // Skip sources that are semantic services (are left in the original item).
          foreach ($vers_lits as $vers_lit_id => $literals)  {
            $new_vers_literal = new IdsKsVersionsLiteral;
            if (!isset($old_vers_literals[$vers_lit_id])) {
              $old_vers_literals[$vers_lit_id] = new IdsKsVersionsLiteral;
              $old_vers_literals[$vers_lit_id]->load($vers_lit_id);
            }
            if ($old_vers_literals[$vers_lit_id]->get('path')) {
              $new_vers_literal->set('path', $old_vers_literals[$vers_lit_id]->get('path'));
            }
            foreach ($literals as $literal) {
              $new_vers_literal->add('literals', $literal);
            }
            if (!$new_vers_literal->isEmptyVersions()) {
              $new_vers_literal->save();
              $new_vers_literals[$source_code][] = $new_vers_literal;
            }
          }
        }
      }
    }
    // Split related items by source.
    if ($related_items = $this->get('related_items')) {
      foreach ($related_items as $related_item) {
        if ($related_item && $related_item->get('source') && $related_item->get('source')->get('code')) {
          $source_code = $related_item->get('source')->get('code');
          if ($source_code == IDSKS_OKHUB_SOURCE_CODE) { // If the source is IDSKS_OKHUB_SOURCE_CODE we keep it in the original item.
            $source_code = $first_source_code;
          }
          $new_rel_items[$source_code][] = $related_item;
        }
      }
    }
    // Split subitems by source.
    if ($subitems = $this->get('subitems')) {
      foreach ($subitems as $subitem) {
        if ($sources_subitem = idskshub_get_item_sources_codes($subitem->getId())) {
          $source_code = array_shift($sources_subitem); // Subitems have only one source.
          if ($source_code == IDSKS_OKHUB_SOURCE_CODE) { // If the source is IDSKS_OKHUB_SOURCE_CODE we keep it in the original item.
            $source_code = $first_source_code;
          }
          $new_subitems[$source_code][] = $subitem;
        }
      }
    }
    // Split track changes by source.
    if ($track_changes = $this->get('track_changes')) {
      foreach ($track_changes as $track_change) {
        if ($track_change && $track_change->get('source') && $track_change->get('source')->get('code')) {
          $source_code = $track_change->get('source')->get('code');
          $new_track_changes[$source_code][] = $track_change;
        }
      }
    }
    foreach ($new_vers_literals as $source_code => $new_vers_literals_source) { // If there are values for more than one source, create new items.
      $new_items[$source_code] = new IdsKsItem;
      $new_items[$source_code]->set('type', $this->get('type'));
      $new_items[$source_code]->set('versions_literals', $new_vers_literals_source);
      if (isset($new_track_changes[$source_code])) {
        $new_items[$source_code]->set('track_changes', $new_track_changes[$source_code]);
      }
      if (isset($root_attrs[$source_code])) {
        $new_items[$source_code]->set('root_attributes', $root_attrs[$source_code]);
      }   
      if (isset($new_subitems[$source_code])) {
        $new_items[$source_code]->set('subitems', $new_subitems[$source_code]);
      }
      // Set display title of new item (based on its type, literals and language)
      $new_items[$source_code]->setDisplayTitle();
      // First save, in order to get an id.
      $new_items[$source_code]->save();
    }
    // For each new item, add links to the other new items just created and the current item and save them.
    foreach ($new_items as $source_code1 => $new_item1) {
      // Add cross-references between new items.
      foreach ($new_items as $source_code2 => $new_item2) {
        if ($source_code1 != $source_code2 && $new_items[$source_code2]->getId() && $same_as_rel->getId() && $okhub_source->getId()) {
          $new_rel_item = new IdsKsRelatedItem;
          $new_rel_item->set('referred_item', $new_items[$source_code2]);
          $new_rel_item->set('relationship', $same_as_rel);
          $new_rel_item->set('source', $okhub_source);
          $new_rel_item->save();
          $new_rel_items[$source_code1][] = $new_rel_item;
        }
      }
      // Add "same as" links between the new items and the current item.
      if ($same_as_rel->isSaved() && $okhub_source->getId()) {
        $new_rel_item = new IdsKsRelatedItem;
        $new_rel_item->set('referred_item', $this);        
        $new_rel_item->set('relationship', $same_as_rel);
        $new_rel_item->set('source', $okhub_source);
        $new_rel_item->save();
        $new_rel_items[$source_code1][] = $new_rel_item;
      }
      if (isset($new_rel_items[$source_code1])) {
        $new_items[$source_code1]->set('related_items', $new_rel_items[$source_code1]);
      }
      // Update new items' related items.
      $new_items[$source_code1]->save();
      // Add "links" from current item to new items.
      if ($new_items[$source_code1]->getId() && $same_as_rel->isSaved() && $okhub_source->getId()) {
        $new_rel_item = new IdsKsRelatedItem;
        $new_rel_item->set('referred_item', $new_items[$source_code1]);
        $new_rel_item->set('relationship', $same_as_rel);
        $new_rel_item->set('source', $okhub_source);
        $new_rel_item->save();
        $new_rel_items[$first_source_code][] = $new_rel_item;
      }
    }
    if (!empty($new_items)) {
      // Remove literals / versions literals.
      if ($this->get('versions_literals')) {
        $first_source_versions_literals = $this->get('versions_literals');
        foreach ($first_source_versions_literals as $pos => $vers_lit) {
          if ($vers_lit->get('literals')) {
            $keep_literals = array();
            foreach ($vers_lit->get('literals') as $old_literal) {
              if ($old_literal && $old_literal->get('source') && ($old_literal->get('source')->get('code') == $first_source_code || in_array($old_literal->get('source')->get('code'), $semantic_codes) || $old_literal->get('source')->get('code') == IDSKS_OKHUB_SOURCE_CODE)) {
                $keep_literals[] = $old_literal;
              }
            }
            if (!empty($keep_literals)) {
              $vers_lit->set('literals', $keep_literals);
              $vers_lit->save();
            }
            else {
              unset($first_source_versions_literals[$pos]);
              $vers_lit->delete(FALSE, FALSE);
            }
          }
        }
        $this->set('versions_literals', array_values(array_filter($first_source_versions_literals)));
      }
      // Change referring items.
      $referring_related_items_ids = IdsKsRelatedItem::getIds(array('referred_item' => $this->getId()));
      foreach ($referring_related_items_ids as $referring_related_item_id) {
        $referring_items_ids = IdsKsItem::getIds(array('related_items' => $referring_related_item_id));
        foreach ($referring_items_ids as $referring_item_id) { // There should be only one.
          $rel_item = new IdsKsRelatedItem();
          $rel_item->load($referring_related_item_id);
          $source_code_rel_item = $rel_item->get('source')->get('code');
          if (isset($new_items[$source_code_rel_item])) {
            $rel_item->set('referred_item', $new_items[$source_code_rel_item]);
            $rel_item->save();
          }
        }
      }
      $this->setDisplayTitle();
      if (isset($root_attrs[$first_source_code])) { // Remove other sources' root attributes.
        $this->set('root_attributes', $root_attrs[$first_source_code]);
      }
      else {
        $this->set('root_attributes', array());
      }
      if (isset($new_rel_items[$first_source_code])) { // Remove other sources' related items.
        $this->set('related_items', $new_rel_items[$first_source_code]);
      }
      else {
        $this->set('related_items', array());
      }
      if (isset($new_subitems[$first_source_code])) { // Remove other sources' subitems.
        $this->set('subitems', $new_subitems[$first_source_code]);
      }
      else {
        $this->set('subitems', array());
      }
      if (isset($new_track_changes[$first_source_code])) { // Remove other sources' track changes.
        $this->set('track_changes', $new_track_changes[$first_source_code]);
      }
      else {
        $this->set('track_changes', array());
      }
      $this->saveItem();
    }
  }

  // Merge two *saved* items of the same type.
  public static function mergeItems($item1, $item2) {
    $merged_item_id = 0;
    if ($item1->isSaved() && $item2->isSaved()) {
      if ($item1->getId() <> $item2->getId()) { // It's not the same item.
        if ($item1->get('type')->getId() == $item2->get('type')->getId()) { // Only merge items if they have the same type.
          if (IdsKsItem::equal($item1, $item2) && $item2->get('versions_literals')) {
            foreach ($item2->get('versions_literals') as $versions_literal2) {
              if ($versions_literal2) {
                $versions_literal2->delete(FALSE, TRUE);
              }
            }
          }
          elseif ($item2->get('versions_literals')) {
            foreach ($item2->get('versions_literals') as $versions_literal2) {
              if ($versions_literal2) {
                $versions_literal1 = $item1->getVersionsLiteralPath($versions_literal2->get('path'));
                if ($versions_literal1 && $versions_literal1->getId()) { // there are literals with this path in item1.
                  foreach ($versions_literal2->get('literals') as $literal2) {
                    if ($literal2 && $literal2->get('source')) {
                      $literals_path_source1 = $versions_literal1->getLiteralVersionsSource($literal2->get('source'));
                      $versions_literal1->addLiteralValue($literal2, $literals_path_source1, 'allow_multiple');
                    }
                  }
                } 
                else {
                  $item1->add('versions_literals', $versions_literal2);
                }
              }
            }
          }          
          $item1->set('track_changes', is_array($item1->get('track_changes')) ? $item1->get('track_changes') : array());
          $item2->set('track_changes', is_array($item2->get('track_changes')) ? $item2->get('track_changes') : array());
          $item1->set('track_changes', array_merge($item1->get('track_changes'), $item2->get('track_changes')));
          foreach ($item2->get('subitems') as $subitem2) {
            if ($subitem2) { 
              if(!$item1->subitemExists($subitem2)) {
                $item1->add('subitems', $subitem2);
              }
              else {
                $subitem2->delete(TRUE, FALSE); // delete its sub-elements but no need to delete from the parent item2, as it will be deleted anyway.
              }
            }
          }
          foreach ($item2->get('related_items') as $related_item2) { // If it's not a circular reference and the items isn't already related to item1, we add the related item.
            if ($related_item2) {
              if ($related_item2->get('referred_item') && $item1->getId() <> $related_item2->get('referred_item')->getId() && !$item1->relatedItemExists($related_item2)) {
                $item1->add('related_items', $related_item2);
              }
              else {
              $related_item2->delete();
              }
            }
          }
          $item1->set('is_merged', 1);
          $item1->setDisplayTitle();
          $item1->saveItem();
          // Change related items pointing to item2 so they now point to item1 (if they werent' already).
          $relitems_parents_item2_ids = IdsKsRelatedItem::getIds(array('referred_item' => $item2->getId()));
          foreach ($relitems_parents_item2_ids as $relitem_parent_item2_id) {
            $relitem_parent_item2 = new IdsKsRelatedItem;
            $relitem_parent_item2->load($relitem_parent_item2_id);
            if ($relitem_parent_item2->get('source') && $relitem_parent_item2->get('relationship')) {
              if ($parent_items_item2_ids = IdsKsItem::getIds(array('related_items' => $relitem_parent_item2_id))) {
                $parent_item2_id = end($parent_items_item2_ids); // There should be exactly one for each relationship.
                $referring_relitems_item1_ids = IdsKsRelatedItem::getIds(array('source' => $relitem_parent_item2->get('source')->getId(), 'relationship' => $relitem_parent_item2->get('relationship')->getId(), 'referred_item' => $item1->getId()));
                $parent_items_item1_ids = IdsKsItem::getIds(array('related_items' => $referring_relitems_item1_ids));
                $parent_item2 = new IdsKsItem;
                $parent_item2->load($parent_item2_id);
                if (!in_array($parent_item2_id, $parent_items_item1_ids)) { // item1 is not referred to by item2's parent item in the same relationship and with the same source.
                  $relitem_parent_item2->set('referred_item', $item1); // the existing related item now points to item1.
                  $relitem_parent_item2->save();
                }
                else { // It's already pointing to item1, we can delete the pointer to item2.
                  $parent_item2_related_items = $parent_item2->get('related_items');
                  foreach ($parent_item2_related_items as $pos => $parent_rel_item) {
                    if ($parent_rel_item && $parent_rel_item->getId() == $relitem_parent_item2_id) {
                      unset($parent_item2_related_items[$pos]);
                    }
                  }
                  $parent_item2->set('related_items', array_filter($parent_item2_related_items));
                  $parent_item2->save();
                  $relitem_parent_item2->delete();
                }
              }
              else {
                $relitem_parent_item2->delete();
              }
            }
            else {
              $relitem_parent_item2->delete();
            }
          }
          $item2->delete(FALSE, FALSE); // We don't delete the literals, changes, subitems and related items, as they are now part of item1.
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
          $mapped_item->set('type', $type);
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
          $mapped_item->set('display_title', $display_title);
        }
        else {
            $mapped_item->set('display_title', "$type->get('name') ($source1->get('name'), $source2->get('name'))");
        }
        $mapped_item->set('is_mapping', 1);
        $mapped_item->saveItem();
      }
      else {
        if (empty($item2_ids)) {
          $mapped_item = new IdsKsItem();
          $mapped_item->load(end($item1_ids));
          $mapped_item->addValues($identifiers[2], $source2);
          $mapped_item->addChanges($source2);
          if ($display_title = $mapped_item->getDisplayFieldValue('title')) {
            $mapped_item->set('display_title', $display_title);
          }
          else {
            $mapped_item->set('display_title', "$type->get('name') ($source1->get('name'), $source2->get('name'))");
          }
          $mapped_item->saveItem();
        }
        else { // both items already exist. we merge them.
          $item1 = new IdsKsItem();
          $item1->load(end($item1_ids));
          $item2 = new IdsKsItem();
          $item2->load(end($item2_ids));
          $item1->set('is_mapping', 1);
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
    if ($source->get('identifiers')) {
      foreach ($source->get('identifiers') as $path_key) {
        if ($type_specific = preg_match('/(\w+)\s*:\s*(\w+)/', $path_key, $matches)) { // The identifier applies only to certain types. This is a quick fix. TODO: Define identifiers based on combinations of source and type.
          $type_name = $matches[1];
          $path_key = $matches[2];
        }
        if (!$type_specific || ($this->get('type') && strtolower($this->get('type')->get('name')) == $type_name)) {
          if ($this->get('versions_literals')) {
            foreach ($this->get('versions_literals') as $versions_literal) {
              if ($versions_literal->get('path') == $path_key) {
                foreach ($versions_literal->get('literals') as $literal) {
                  if ($literal && $literal->get('source')->getId() == $source->getId()) {
                    $values_identifiers[$path_key] = $literal->get('value');  
                    break; // as it's a key for this source there should be only one.
                  }
                }
                break;        
              }
            }
          }
        }
      }
    }
    if (!empty($values_identifiers)) {
      $items_ids = IdsKsItem::retrieveItemIdsByKeys($values_identifiers, $source->get('compound_key'), $source, $this->get('type'));
    }
    return $items_ids;
  }

  // Returns an array of item ids that are duplicated in relation to the current item, based on the values of the type's ids.
  public function getDuplicatesByType() {
    $items_ids = array();
    if ($this->get('type')) {
      $type = $this->get('type');
      if ($type->get('title_identifier') && $this->get('display_title')) {
        $items_ids = IdsKsItem::getIds(array('display_title' => $this->get('display_title'), 'type' => $type->getId()));
      }
      if (empty($items_ids)) {
        $values_identifiers = array();
        if ($type->get('identifiers')) {
          foreach ($type->get('identifiers') as $path_key) {
            if ($this->get('versions_literals')) {
              foreach ($this->get('versions_literals') as $versions_literal) {
                if ($versions_literal->get('literals') && $versions_literal->get('path') == $path_key) {
                  $literal = end($versions_literal->get('literals')); // It is assumed that if two items are the same, the values for the keys from all the sources will be the same.
                  if ($literal && !is_null($literal->get('value'))) {
                    $values_identifiers[$path_key] = $literal->get('value');  
                    break;        
                  }
                }
              }
            }
          }
        }
        if (!empty($values_identifiers)) {
          $items_ids = IdsKsItem::retrieveItemIdsByKeys($values_identifiers, $type->get('compound_key'), NULL, $type);
        }
      }
    }
    return $items_ids;
  }

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
        $items_ids = IdsKsItem::getIds($filters);
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
    if ($this->get('versions_literals')) {
      foreach ($this->get('versions_literals') as $versions_literal) {
        if ($versions_literal && $versions_literal->get('path') == $path) {
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
      if (!is_array($this->get('versions_literals'))) {
        $this->set('versions_literals', array());
      }
      $found_versions_literal = new IdsKsVersionsLiteral();
      $found_versions_literal->set('path', $mapped_path);
      $this->add('versions_literals', $found_versions_literal);
    }
    return $found_versions_literal;
  }

  // Deletes all values in the item for the indicated source.
  public function deleteLiteralsSource($source) {
    if ($this->get('versions_literals')) {
      $versions_literals = $this->get('versions_literals');
      foreach ($versions_literals as $key => $versions_literal) {
        $delete_literals_ids = array();
        if ($versions_literal->get('literals')) {
          foreach ($versions_literal->get('literals') as $literal) {
            if ($literal && $literal->get('source')->getId() == $source->getId()) {
              $delete_literals_ids[] = $literal->getId();
            }
          }
          $versions_literal->deleteLiteralsByIds($delete_literals_ids);
        }
        if (!$versions_literal->get('literals')) {
          unset($versions_literals[$key]);
          $versions_literal->delete(FALSE, FALSE);          
        }
      }
      $this->set('versions_literals', array_values(array_filter($versions_literals)));
    }
  }

  // Removes all subitems corresponding to the source from the subitems list of the item.
  public function removeSubitemsSource($source) {
    if ($this->get('subitems')) {
      $subitems = $this->get('subitems');
      foreach ($subitems as $key => $subitem) {
        if ($subitem && !$subitem->isTypedItem()) { // if it's not a typed subitem, it's specific of this item.
          $subitem->deleteLiteralsSource($source);
          $subitem->removeSubitemsSource($source);
          if ($subitem->isEmptyItem()) {
            $subitem->delete();
          }
        }
        unset($subitems[$key]);
      }
      $this->set('subitems', array_values(array_filter($subitems)));
    }
  }

  // Removes all root attirbutes corresponding to the source from the item.
  public function removeRootAttributesSource($source) {
    if ($this->get('root_attributes')) {
      $root_attributes = $this->get('root_attributes');
      foreach ($root_attributes as $key => $root_attribute) {
        if (preg_match('/^\['.$source->get('code').'\]:/', $root_attribute->get('name'))) {
          unset($root_attributes[$key]);
        }
      }
      $this->set('root_attributes', array_values(array_filter($root_attributes)));
    }
  }

  // Removes all related items corresponding to the source from the related_items list of the item.
  public function removeRelatedItemsSource($source) {
    if ($this->get('related_items')) {
      $related_items = $this->get('related_items');
      foreach ($related_items as $key => $related_item) {
        if ($related_item && $related_item->get('source')->getId() == $source->getId()) {
          $related_item->delete();
          unset($related_items[$key]);
        }
      }
      $this->set('related_items', array_values(array_filter($related_items)));
    }
  }

  public function subitemExists($new_subitem) {
    $found = FALSE;
    if ($this->get('subitems')) {
      foreach ($this->get('subitems') as $subitem) {
        if (IdsKsItem::equal($new_subitem, $subitem)) {
          $found = TRUE;
          break;
        }
      }
    }
    return $found;
  }

/*
  public function isRelatedItem($item) {
    $rels_ids = IdsKsRelatedItem::getIds(array('referred_item' => $item->getId()));
    $items_ids = IdsKsItem::getIds(array('related_items' => $rels_ids));
    $is_related = in_array($this->getId(), $items_ids);
    return $is_related;
  }
*/

  public function getRelatedItem($item) {
    $found_rel_item = NULL;
    if ($this->get('related_items')) {
      foreach ($this->get('related_items') as $rel_item) {
        if ($rel_item->get('referred_item') && $rel_item->get('referred_item')->getId() == $item->getId()) {
          $found_rel_item = $rel_item;
          break;
        }
      }
    }
    return $found_rel_item;
  }

  public function relatedItemExists($new_related_item) {
    $found = FALSE;
    $related_items_referred_item = ($new_related_item->get('referred_item')) ? IdsKsRelatedItem::getIds(array('referred_item' => $new_related_item->get('referred_item')->getId())) : array();
    $related_items_item = IdsKsItem::getChildrenIds($this->getId(), 'related_items');
    $candidate_related_items_ids = array_intersect($related_items_item, $related_items_referred_item);
    foreach ($candidate_related_items_ids as $related_item_id) {
      $related_item = new IdsKsRelatedItem();
      $related_item->load($related_item_id);
      if (($related_item->get('source')->getId() == $new_related_item->get('source')->getId()) && ($related_item->get('relationship')->getId() == $new_related_item->get('relationship')->getId())) {
        $found = $related_item;
        break;
      }
    }
    return $found;
  }

  // Populates an item with the values present in the XML passed. Main function used to import items.
  public function populateItemXML($xml_item, $item_path, $source, $format, $attributes_added = FALSE, $preprocessed = FALSE, $collected_attributes = array()) {
    $versions_literals = array();
    $root_attributes_xml = (array) $xml_item->attributes();
    $root_attributes = array();
    if (!empty($root_attributes_xml)) {
      if (!$attributes_added) {
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
        if (!IdsKsMapping::isIgnore($xml_path, $attributes_value, $format, $source, $this->get('type'))) { // Not set to IGNORE
          if (($callback = IdsKsMapping::isPreprocess($xml_path, $attributes_value, $format, $source, $this->get('type'))) && !$preprocessed) { // Is PREPROCESS.
            if (is_callable($callback)) {
              $callback($xml_value, $source, $xml_path, $collected_attributes); // Can change $xml_value and $xml_path.
              if (is_a($xml_value, 'SimpleXMLElement')) {
                $this->populateItemXML($xml_value, $xml_path, $source, $format, TRUE, TRUE, $collected_attributes);
              }
              else {
                  trigger_error(t('Callback function @callback did not return a valid SimpleXMLElement and the element @field_path was ignored.', array('@callback' => $callback, '@field_path' => $xml_path)));
              }
            }
            else {
              trigger_error(t('Callback function @callback could not be invoked and the element @field_path was ignored. Please make sure that the function is defined in idskshub.callback.inc.', array('@callback' => $callback, '@field_path' => $xml_path)));
            }
          }
          else { // Not PREPROCESS mapping.
            if (!$mappings = IdsKsMapping::getMappings($xml_path, $attributes_value, $format, $source, $this->get('type'), '', 'PREPROCESS')) { // There are other mappings, excluding preprocess. 
              $mappings[] = new IdsKsMapping;
            }
            foreach ($mappings as $mapping) {
              if ($mapping->get('path_destination')) {
                $new_path_mapped = $mapping->get('path_destination');
              }
              if ($mapping->get('action') && $mapping->get('action') == 'RELATED_ITEM' && $mapping->get('relationship')) { // Create a new relationship to a related item.
                $xml_value_related = clone $xml_value;
                if ($mapping->get('path_destination')) {
                  $xml_value_related->addChild($mapping->get('path_destination'), (string)$xml_value_related);
                }
                elseif (!$xml_value_related->count()) { // If it's not a structured value and no mapping field has been defined, we add a default field.
                  $xml_value_related->addChild(IDSKS_DEFAULT_MAPPED_FIELD, (string)$xml_value_related); //TODO: Check the type's title fields, if they are defined.
                }
                $serialized_referred_item = $xml_value_related->asXML();
                $save_referred_item = FALSE;
                $new_referred_item = new IdsKsItem();
                $new_referred_item->set('type', $mapping->get('relationship')->get('item_type'));
                $new_referred_item->populateItemXML($xml_value_related, $xml_path, $source, $format);
                $referred_item_ids = $new_referred_item->getDuplicatesBySource($source); // Look by the source's ids.
                if ($found_by_source = !empty($referred_item_ids)) {
                  $update_existing = FALSE; // We do not update related items from the same source "indirectly".
                }
                else {
                  $referred_item_ids = $new_referred_item->getDuplicatesByType(); // Look by type.
                  $update_existing = $new_referred_item->get('type')->get('update_existing');
                }
                if (empty($referred_item_ids)) { // It's a new item of this type. We save it.
                  $referred_item = $new_referred_item;
                  $save_referred_item = TRUE;
                }
                else { // Matching items already exist. 
                  if ($found_by_source || $update_existing) {
                    $referred_item_id = end($referred_item_ids);
                    $referred_item = new IdsKsItem();
                    $referred_item->load($referred_item_id);              
                    if ($update_existing && !$referred_item->hasContentFromSource($source->getId())) { // We only update it if it doesn't already have content from this source.
                      $referred_item->updateSourceValues($new_referred_item, $source);
                      $save_referred_item = TRUE;
                    }
                  }
                  else { // We do not update the content but add "same as" links to matching items.
                    // Get source corresponding to the OKHub;
                    $okhub_source = new IdsKsSource;
                    $okhub_source->set('code', IDSKS_OKHUB_SOURCE_CODE);
                    $okhub_source->load();
                    $referred_item = $new_referred_item;
                    $rel_items = array();
                    $same_as_rel = new IdsKsRelationship;
                    //----- Leaving this temporarily for backward compatibility, until the relationships are updated in the live server ---------------
                    if (!defined('IDSKS_DEFAULT_MAPPING_REL_CATEGORY')) {
                      $rel_code = 'same_as_' . strtolower($referred_item->get('type')->get('name'));
                    }
                    //---------------------------------------------------------------------------------------------------------------------------------
                    else {
                      if ($referred_item->get('type')->get('is_category')) {
                        $rel_code = IDSKS_DEFAULT_MAPPING_REL_CATEGORY;
                      }
                      else {
                        $rel_code = IDSKS_DEFAULT_MAPPING_REL_ASSET;
                      }
                    }
                    if ($rels_ids = IdsKsRelationship::getIds(array('code' => $rel_code, 'item_type' => $referred_item->get('type')->getId()))) {
                      // rel. code and item type should be a key, so there should be only one.
                      $same_as_rel->load($rels_ids[0]);
                      if ($same_as_rel->isSaved()) {
                        foreach ($referred_item_ids as $referred_item_id) {
                          $rel_item = new IdsKsItem;
                          $rel_item->load($referred_item_id);
                          $new_rel_item = new IdsKsRelatedItem;
                          $new_rel_item->set('referred_item', $rel_item);
                          $new_rel_item->set('relationship', $same_as_rel);
                          $new_rel_item->set('source', $okhub_source);
                          if (!$referred_item->relatedItemExists($new_rel_item)) {
                            $rel_items[] = $new_rel_item;
                            $new_rel_item->save();
                            $referred_item->add('related_items', $new_rel_item);
                          }
                        }
                        if (!empty($rel_items)) {
                          $save_referred_item = TRUE;
                        }
                      }
                    }
                  }
                }
                if ($save_referred_item) {
                  $referred_item->addChanges($source, $serialized_referred_item);
                  $referred_item->saveSubitems();
                  $referred_item->saveRelatedItems();
                  $referred_item->saveItem();
                  $referred_item->createInverseRelatedItems();
                }
                // Add the new related item to the item being imported.
                $related_item = new IdsKsRelatedItem();
                $related_item->set('source', $source);
                $related_item->set('relationship', $mapping->get('relationship'));
                $related_item->set('referred_item', $referred_item);
                $related_item->set('path', $xml_path);
                $this->add('related_items', $related_item);
              }
              elseif ($xml_value->count() > 0) { // It's a structured value. Not RELATED_ITEM.
                $is_an_array = FALSE;
                if (!$skip_tag = ($mapping->get('action') && $mapping->get('action') == 'SKIP')) { // The element should not be skipped.
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
                if ($skip_tag || $is_an_array) { // It's an array or a "skip tag". We don't add another structure, but add each element to the parent item.
                  $this->populateItemXML($xml_value, $xml_path, $source, $format, TRUE, $preprocessed, $collected_attributes);
                }
                else { // It is a structured subitem.
                  $subitem = new IdsKsItem();
                  $subitem->set('type', IdsKsType::getGenericType());
                  $subitem->set('path', $new_path_mapped);
                  $subitem->populateItemXML($xml_value, $xml_path, $source, $format);
                  $this->add('subitems', $subitem);
                }
              }
              else { // It is a literal. We add it, except there's also a relationship defined for the path.
                if (!IdsKsMapping::getMappings($xml_path, $attributes_value, $format, $source, $this->get('type'), 'RELATED_ITEM')) {
                  if (!isset($versions_literals[$new_path_mapped])) { // retrieve exising versions literal corresponding to the path (or create it if it doesn't exist).
                    $versions_literals[$new_path_mapped] = $this->getVersionsLiteralPath($new_path_mapped);
                  }
                  $versions_literals[$new_path_mapped]->addLiteralXML($xml_value, $source, $xml_path, $attributes_value); // The literal value is added to the item.
                }
              }
            }
          }
        }
      }
    }
    if ($this->get('type')) {
      if ($display_title = $this->getDisplayFieldValue('title')) {
        $this->set('display_title', $display_title);
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
        $source->set('code', $service_code);
        $source->load();
        if ($source->getId() && $source->get('is_semantic')) {
          $semantic_entities = idskshub_semantic_services_call($service_code, $data);
          foreach ($semantic_entities as $path_entity_name => $entities) {
            $path_mapped = IdsKsMapping::getMappingDestination($path_entity_name);
            $versions_literal = $this->getVersionsLiteralPath($path_mapped);
            foreach ($entities as $entity) {
              $new_literal = new IdsKsLiteral();
              $new_literal->set('source', $source);
              $new_literal->set('value', $entity); 
              $new_literal->set('original_path', $path_entity_name); 
              if (!($versions_literal->inLiterals($new_literal))) {
                $versions_literal->add('literals', $new_literal);
              }
            }
          }
        }
      }
    }
  }

  // Indicates if the item is "typed" (corresponds to one of the main types defined in the hub).
  public function isTypedItem() {
    return ($this->get('type')->get('name') != IDSKS_GENERIC_TYPE_NAME);
  }

  // Updates an existing item with the values of the temporary item $new_item. *Does not save the item*.
  public function updateSourceValues($new_item, $source, $update_mode = 'incremental', $allow_multiple = 'allow_multiple') { // updates values in $this with the ones in $new_item.
    // Literals
    if ($update_mode == 'replace' && $new_item->get('versions_literals')) {
      $this->deleteLiteralsSource($source);
      if (!$this->get('versions_literals')) {
        $this->set('versions_literals', $new_item->get('versions_literals'));
      }
      else {
        $this->set('versions_literals', array_merge(array_filter($this->get('versions_literals')), $new_item->get('versions_literals')));
      }
    }
    else {
      // Add literals from the new_item's source (without duplicated values).
      if ($new_item->get('versions_literals')) {
        foreach ($new_item->get('versions_literals') as $new_versions_literal) {
          $versions_literal = $this->getVersionsLiteralPath($new_versions_literal->get('path'));
          $literals_path_source = $versions_literal->getLiteralVersionsSource($source);
          if ($new_versions_literal->get('literals')) { // Not necessary to filter by source because $new_item just has values from this source.
            foreach ($new_versions_literal->get('literals') as $new_literal) {
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
    if ($new_item->get('subitems')) {
      foreach ($new_item->get('subitems') as $new_subitem) {
        if (!($this->subitemExists($new_subitem)) && !($new_subitem->isEmptyItem())) {
          //$new_subitem->saveItem(TRUE, FALSE);
          $this->add('subitems', $new_subitem);
        }
      }
    }
    if ($new_item->get('related_items')) {
      foreach ($new_item->get('related_items') as $new_related_item) {
        if (($this->getId() <> $new_related_item->get('referred_item')->getId()) && !$this->relatedItemExists($new_related_item)) {
          //$new_related_item->save();
          $this->add('related_items', $new_related_item);
        }
      }
    }
    if ($new_item->get('root_attributes')) {
      foreach ($new_item->get('root_attributes') as $new_attribute) {
        if (array_udiff($this->get('root_attributes'), array($new_attribute), 'idskshub_diff')) {
          $this->add('root_attributes', $new_attribute);
        }
      }
    }
    if (!$this->display_title && $new_item->display_title) { // TODO. Assign based on source's "priorities".
      $this->set('display_title', $new_item->display_title);
    }
  }

} // class IdsKsItem

