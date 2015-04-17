<?php
// $Id$

/**
 * @file
 * IDS KS Attribute class.
 *
 */

class IdsKsAttribute extends IdsKsObject {
  public $name;
  public $value;

  public static function getAttribute($name, $value) {
    $attribute = new IdsKsAttribute();
    if ($name && $value) {
      $attribute->name = $name;
      $attribute->value = $value;
      $attribute->load();
      if (!($attribute->getId())) {
        $attribute->save();
      }
    }
    return $attribute;
  }

  public static function getAttributesArray($array_attributes) {
    $attributes = array();
    if (isset($array_attributes['@attributes'])) {
      $array_attributes = $array_attributes['@attributes'];
    }
    foreach ($array_attributes as $name => $value) {
      $name = trim($name);
      $value = trim($value);
      if ($name && $value) {
        $name = idskshub_restore_namespace($name);
        $attributes[] = IdsKsAttribute::getAttribute($name, $value);
      }
    }
    return $attributes;
  }

  public static function getAttributesString($list_attributes) {
    $attributes = array();
    $array_attributes = explode(',', $list_attributes);
    foreach ($array_attributes as $string_attribute) {
      $attr = explode('=', $string_attribute);
      if (isset($attr[0]) && isset($attr[1])) {
        $name = trim($attr[0]);
        $value = trim($attr[1]);
        if ($name && $value) {
          $attributes[] = IdsKsAttribute::getAttribute($name, $value);
        }
      }
    }
    return $attributes;
  }

} // class IdsKsAttribute
