<?php
// $Id$

/**
 * @file
 * IDS KS Literal class.
 *
 */

class IdsKsLiteral extends IdsKsObject {
  public $source;
  public $attributes;
  public $value;
  public $original_path;

  public function addAttributesLiteral($attributes_objects) {
    foreach ($attributes_objects as $attribute) {
      $this->attributes[] = $attribute;
    }
  }

  public static function deleteLiteral($id, $delete_from_parent_versions = TRUE) {
    $lit = new IdsKsLiteral();
    $lit->load($id);
    if ($lit->isSaved()) {
      $lit->delete($delete_from_parent_versions);
      return TRUE;
    }
    else {
      return FALSE;
    }
  }

  public function delete($delete_from_parent_versions = TRUE) {
    $lit_id = $this->getId();
    parent::delete();
    if ($delete_from_parent_versions) {
      $parent_versions_ids = IdsKsVersionsLiteral::getParentsIds(array('literals' => $lit_id));
      if (!empty($parent_versions_ids)) {
        $vlit = new IdsKsVersionsLiteral();
        $vlit->load($parent_versions_ids[0]);
        $literals = array();
        foreach ($vlit->literals as $child) {
          if (isset($child)) {
            $literals[] = $child;
          }
        }
        if (empty($literals)) {
          $vlit->delete(TRUE);
        }
        else {
          $vlit->literals = $literals;
          $vlit->save();
        }
      }
    }
  }

} // class IdsKsLiteral
