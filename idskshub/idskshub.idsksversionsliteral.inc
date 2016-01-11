<?php
// $Id$

/**
 * @file
 * IDS KS Path class.
 *
 */

class IdsKsVersionsLiteral extends IdsKsObject {
  public $literals;
  public $path;

  public function save() {
    // Make sure not to leave NULL literals.
    $this->set('literals', array_filter($this->get('literals')));
    parent::save();
  }

  // Get the literals corresponding to the passed source.
  public function getLiteralVersionsSource($source) {
    $literals_source = array();
    if ($this->get('literals')) {
      foreach ($this->get('literals') as $literal) {
        if ($literal && $literal->get('source') && $literal->get('source')->getId() == $source->getId()) {
          $literals_source[] = $literal;
        }
      }
    }
    return $literals_source;
  }

  public function isEmptyVersions() {
    return (!$this->get('literals'));
  }

  // Get the literal versions objects corresponding to the literal value, path (and source, if present).
  public static function getLiteralsVersions($path, $value, $source = NULL) {
    $found_versions = array();
    $lit = new IdsKsLiteral();
    $lit->set('value', $value);
    if (isset($source)) {
      $lit->set('source', $source);
    }
    $literals_value = $lit->getObjectsByProperties();
    if (!empty($literals_value)) {
      foreach ($literals_value as $literal) {
        $versions_literal_ids = IdsKsVersionsLiteral::getIds(array('literals' => $literal->getId()));
        foreach ($versions_literal_ids as $versions_literal_id) {
          $versions = new IdsKsVersionsLiteral();
          $versions->load($versions_literal_id);
          if ($versions->get('path') && $versions->get('path') == $path) {
            $found_versions[] = $versions;
          }
        }
      }
    }
    return $found_versions;
  }

  // Add literal $new_literal to the current literal's versions. Check for existing literals of this source if they exits and should be replaced.
  // Returns an array of the ids of the literals to be deleted (as they are replaced by the new value for the path/source) (if $allow_multiple == 'replace').
  public function addLiteralValue($new_literal, $literals_path_source, $allow_multiple) {
    $new_literal->set('value', trim($new_literal->get('value')));
    $literals_to_delete = array();
    $literals_to_add = array();
    if (empty($literals_path_source)) {
      $this->add('literals', $new_literal);
    }
    else {
      foreach ($literals_path_source as $literal) { // for each literal with this path and belonging to this source.
        if ($literal) {
          $same_value = (trim($literal->get('value')) == $new_literal->get('value'));
          $included_attrs_literal = ((!$literal->get('attributes') && !$new_literal->get('attributes')) || !(array_udiff($literal->get('attributes'), $new_literal->get('attributes'), 'idskshub_diff')));
          $included_new_attrs_literal = ((!$literal->get('attributes') && !$new_literal->get('attributes')) || !(array_udiff($new_literal->get('attributes'), $literal->get('attributes'), 'idskshub_diff')));
          $same_attributes = ($included_attrs_literal && $included_new_attrs_literal);
          if (!$same_value || !$same_attributes) {
            if ($allow_multiple == 'replace') {
              $delete = TRUE;
              $add_new = TRUE;
            }
            else {
              if ($same_value) { // same value - different attributes.
                if ($included_attrs_literal) { // the new attributes are more specific. 
                  $delete = TRUE;
                  $add_new = TRUE;
                }
                elseif ($included_new_attrs_literal) { // the existing attributes are more specific.
                  $delete = FALSE;
                  $add_new = FALSE;
                }
                else { // the attributes are completely different.
                  $delete = FALSE;
                  $add_new = TRUE;
                }
              } // different value and replace == 'allow_multiple'.
              else {
                $delete = FALSE;
                $add_new = TRUE;              
              }
            }
          }
          else { // same value and attributes. do nothing.
            $delete = FALSE;
            $add_new = FALSE;  
          }
          if ($delete) {
              $literals_to_delete[] = $literal->getId();
          }
          if ($add_new) {
            $literals_to_add[] = $new_literal;
          }
        }
      }
      if (!empty($literals_to_delete)) {
        $this->deleteLiteralsByIds($literals_to_delete);
      }
      if (!empty($literals_to_add)) {
        foreach ($literals_to_add as $new_lit) {
          if (!($this->inLiterals($new_lit))){ // check if it's not another value for the same literal that already exists.
            $this->add('literals', $new_lit);
          }
        }
      }
    }
  }

  // Check if a literal is already in the list of versions with the same or more specific attributes.
  public function inLiterals($literal) {
    $exists = FALSE;
    if ($this->get('literals')) {
      foreach ($this->get('literals') as $lit) {
        if ($lit && $lit->get('source')) {
          if ($lit->get('source')->getId() == $literal->get('source')->getId() && !$lit->get('value') && !$literal->get('value') || $lit->get('value') === $literal->get('value')) {
            if ($literal->get('attributes') || $lit->get('attributes')) {
              // if the new literal's attributes are included in the existing literal, we consider that it already exists.
              $exists = !(array_udiff($literal->get('attributes'), $lit->get('attributes'), 'idskshub_diff'));
            }
            else {
              $exists = TRUE;
            }
          }
          if ($exists) {
            break;
          }
        }
      }
    }
    return $exists;
  }

  public static function deleteVersionsLiteral($id, $delete_from_parent_item = TRUE) {
    $vlit = new IdsKsVersionsLiteral();
    $vlit->load($id);
    if ($vlit->isSaved()) {
      $vlit->delete($delete_from_parent_item);
      return TRUE;
    }
    else {
      return FALSE;
    }
  }

  public function delete($delete_from_parent_item = TRUE, $delete_literals = TRUE) {
    if ($this->get('literals') && $delete_literals) {
      foreach ($this->get('literals') as $literal) {
        if (isset($literal)) {
          $literal->delete(FALSE);
        }
      }
    }
    if ($delete_from_parent_item) {
      $parent_items_ids = IdsKsItem::getIds(array('versions_literals' => $this->getId()));
      if (!empty($parent_items_ids)) {
        $item = new IdsKsItem();
        $item->load($parent_items_ids[0]);
        $versions_literals = array();
        foreach ($item->get('versions_literals') as $child) {
          if ($child->getId() <> $this->getId()) {
            $versions_literals[] = $child;
          }
        }
        $item->set('versions_literals', $versions_literals);
        $item->save();
      }
    }
    parent::delete();
  }

  // Delete the passed literals and remove them from the list of versions.
  // If $delete_literals = FALSE we don't delete the literal objects as they are referenced by another versions literal (after merging items or applying mappings, for instance).
  public function deleteLiteralsByIds($literals_ids, $delete_literals = TRUE) {
    $new_literals = array();
    foreach ($this->get('literals') as $literal) {
      if (isset($literal) && !in_array($literal->getId(), $literals_ids)) {
        $new_literals[] = $literal;
      }
    }
    $this->set('literals', $new_literals);
    if ($delete_literals) {
      IdsKsLiteral::deleteMultiple($literals_ids);
    }
  }

  public function deleteLiteralsBySourceId($source_id) {
    $new_literals = array();
    $delete_literals_ids = array();
    if (is_array($this->get('literals'))) {
      foreach ($this->get('literals') as $literal) {
        if (isset($literal)) {
          if ($literal->get('source') && $literal->get('source')->getId() <> $source_id) {
            $new_literals[] = $literal;
          }
          else {
            $delete_literals_ids[] = $literal->getId();
          }
        }
      }
      $this->set('literals', $new_literals);
      IdsKsLiteral::deleteMultiple($delete_literals_ids);
    }
  }

  // Add a new literal version.
  public function addLiteralXML($xml_value, $source, $original_path = '', $attributes = array()) {
    $value = (string) $xml_value;
    $value = preg_replace('/[[:cntrl:]]/', ' ', $value); // remove control characters.
    $value = trim($value);
    if (!empty($value)) {
      $new_literal = new IdsKsLiteral();
      $new_literal->set('source', $source);
      if ($original_path) {
        $new_literal->set('original_path', $original_path);
      }
      if ($attributes) {
        $new_literal->addAttributesLiteral($attributes);
      }
      $new_literal->set('value', $value);
      if (!$this->inLiterals($new_literal)) {
        $this->add('literals', $new_literal);
      }
    }
  }

}  // class IdsKsVersionsLiteral
