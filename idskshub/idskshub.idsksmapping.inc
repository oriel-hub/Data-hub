<?php
// $Id$

/**
 * @file
 * IDS KS Mapping class.
 *
 */

class IdsKsMapping extends IdsKsObject {
  public $format;
  public $source;
  public $type;
  public $path_origin;
  public $attributes_origin;
  public $path_destination;
  public $relationship;
  public $action;

  public static function createMappingsFile($fid, $format_code, $source_code, $type_name, $replace) {
    $error = FALSE;
    if ($fid) {
      $file = file_load($fid);
      $file_path = drupal_realpath($file->uri);
    }
    if ($handle = fopen($file_path, 'r')) {
      if ($replace === 'replace') {
        IdsKsObject::deleteAll('IdsKsMapping');
      }
      while (list($format_code_file, $source_code_file, $type_name_file, $path_orig, $attributes_origin, $path_dest, $action, $relationship_code) = fgetcsv($handle)) {
        if ($format_code_file) { // there is a format in the file.
          $format_code_mapping = $format_code_file;
        }
        else {
          $format_code_mapping = $format_code;
        }
        if ($source_code_file) { // there is a source in the file.
          $source_code_mapping = $source_code_file;
        }
        else {
          $source_code_mapping = $source_code;
        }
        if ($type_name_file) { // there is a type in the file.
          $type_name_mapping = $type_name_file;
        }
        else {
          $type_name_mapping = $type_name;
        }
        $relationship_id = end(IdsKsRelationship::getIds(array('code' => $relationship_code)));
        trim($path_orig, '/');
        trim($path_dest, '/');
        if (idskshub_check_path($path_orig, TRUE) && (idskshub_check_path($path_dest) || (!$path_dest && in_array($action, array_keys(idskshub_mapping_actions()))))) {
          $mapping = self::createMappingFromStrings($path_orig, $path_dest, $format_code_mapping, $source_code_mapping, $type_name_mapping, $attributes_origin, $relationship_id, $action);
          if (!($mapping->getId())) {
            $error = TRUE;
            drupal_set_message(t('The mapping @path_orig -> @path_dest could not be created', array('@path_orig' => $path_orig, '@path_dest' => $path_dest)), 'warning');
          }
        }
        else {
          $error = TRUE;
          drupal_set_message(t('The mapping @path_orig -> @path_dest could not be created. The paths are not valid.', array('@path_orig' => $path_orig, '@path_dest' => $path_dest)), 'warning');
        }
      }
      if (!$error) {
        drupal_set_message(t('The mappings were created.'));
      }
    }
  }

  public static function createMappingFromStrings($path_origin, $path_destination, $format_code='', $source_code='', $type_name='', $attributes_origin='', $relationship_id=0, $action='') {
    $mapping = new IdsKsMapping();
    $mapping->path_origin = $path_origin;
    $attributes = array();
    if ($attributes_origin) {
      $attributes = IdsKsAttribute::getAttributesString($attributes_origin);
      if (!empty($attributes)) {
        $mapping->attributes_origin = $attributes;
      }
    }
    $mappings = $mapping->getObjectsByProperties();
    if (!empty($mappings)) {
      // There are mappings for this origin. We have to see if we keep them or delete them if they are covered by the new one.
      foreach ($mappings as $old_mapping) {
        // If it exists for this format and source or this one is more generic we delete the old one.
        $replace_format = (empty($format_code) || (isset($old_mapping->format) && $old_mapping->format->code === $format_code));
        $replace_source = (empty($source_code) || (isset($old_mapping->source) && $old_mapping->source->code === $source_code));
        $replace_type = (empty($type_name) || (isset($old_mapping->type) && $old_mapping->type->name === $type_name));
        if  ($replace_format && $replace_source && $replace_type) {
          // If the attributes are the same.
          if (empty($attributes) && empty($old_mapping->attributes_origin)) {
            $old_mapping->delete();
          }
          elseif (isset($old_mapping->attributes_origin)) {
            sort($attributes);
            sort($old_mapping->attributes_origin);
            if ($old_mapping->attributes_origin == $attributes) {
              $old_mapping->delete();
            }
          }
        }
      }
    }
    if ($format_code) {
      $format = new IdsKsFormat();
      $format->code = $format_code;
      $format->load();
      if ($format->getId()) {
        $mapping->format = $format;
      }
    }
    if ($source_code) {
      $source = new IdsKsSource();
      $source->code = $source_code;
      $source->load();
      if ($source->getId()) {
        $mapping->source = $source;
      }
    }
    if ($type_name) {
      $type = new IdsKsType();
      $type->name = $type_name;
      $type->load();
      if ($type->getId()) {
        $mapping->type = $type;
      }
    }
    if ($relationship_id) {
      $relationship = new IdsKsRelationship();
      $relationship->load($relationship_id);
      if (isset($relationship->type)) {
        $mapping->relationship = $relationship;
      }
    }
    if (in_array($action, array_keys(idskshub_mapping_actions()))) {
      $mapping->action = $action;
    }
    $mapping->path_destination = $path_destination;
    $mapping->save();
    return $mapping;
  }

  public static function getMapping($path_origin, $attributes = array(), $format = NULL, $source = NULL, $type = NULL) {
    $mapping_found = NULL;
    $mapping = new IdsKsMapping();
    $mapping->path_origin = $path_origin;
    $mappings = $mapping->getObjectsByProperties();
    if (empty($mappings)) { // mapping not found. check if a mapping exists with a wildcard.
      $tag = array_pop(explode('/', $path_origin));
      $mapping->path_origin = "*/$tag";
      $mappings = $mapping->getObjectsByProperties();
    }
    if (!empty($mappings)) {
      foreach ($mappings as $mapping) {
        $format_matches = (empty($mapping->format) || ($mapping->format == $format));
        $source_matches = (empty($mapping->source) || ($mapping->source == $source));
        $type_matches = (empty($mapping->type) || ($mapping->type == $type));
        if ($format_matches && $source_matches && $type_matches) {
          if (!isset($mapping->attributes_origin) || !(array_udiff($mapping->attributes_origin, $attributes, 'idskshub_diff'))) { // the attributes of the mapping are included in the passed attributes.
            $mapping_found = $mapping; // the destination is found. we stop looking.
            break;
          }
        }
      }
    }
    return $mapping_found;
  }

  public static function getMappingDestination($path_origin, $attributes = array(), $format = NULL, $source = NULL, $type = NULL) {
    $mapped = $path_origin;
    if ($mapping = IdsKsMapping::getMapping($path_origin, $attributes, $format, $source, $type)) {
      $mapped = $mapping->path_destination;
    }
    return $mapped;
  }

  public function getLabel() {
    $attributes_str = '';
    $format = (isset($this->format)) ? $this->format->code : '*';
    $source = (isset($this->source)) ? $this->source->code : '*';
    $type = (isset($this->type)) ? $this->type->name : '*';
    $path_origin = (isset($this->path_origin)) ? $this->path_origin : '*';
    $path_destination = (isset($this->path_destination)) ? $this->path_destination : '*';
    if (isset($this->attributes_origin)) {
      $attributes = array();
      foreach ($this->attributes_origin as $attribute) {
        $attributes[] = $attribute->name . '=' . $attribute->value;
      }
      if ($attributes) {
        $attributes_str = ' [' . implode(',', $attributes) . ']';
      }
    }
    return $format . ':' . $source . ':' . $type . ':' . $path_origin  . $attributes_str . ' --> ' . $path_destination;
  }

} // class IdsKsMapping
