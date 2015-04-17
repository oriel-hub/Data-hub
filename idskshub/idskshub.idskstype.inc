<?php
// $Id$

/**
 * @file
 * IDS KS Type class.
 *
 */

class IdsKsType extends IdsKsObject {
  public $name;
  public $description;
  public $identifiers; // Now allowing just one set. For example: 'document' => <root>:'title', <root>:'author'
  public $compound_key; // Indicates if identifiers should be connected by AND.
  public $title_paths;
  public $description_paths;

  public static function getGenericType() {
    $type = new IdsKsType();
    $type->name = IDSKS_GENERIC_TYPE_NAME;
    $type->load();
    if (!($type->getId())) {
      $type->compound_key = 0;
      $type->save();
    }
    return $type;
  }

} // class IdsKsType

