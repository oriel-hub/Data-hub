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
    $mapping->set('path_origin', $path_origin);
    $attributes = array();
    if ($attributes_origin) {
      $attributes = IdsKsAttribute::getAttributesString($attributes_origin);
      if (!empty($attributes)) {
        $mapping->set('attributes_origin', $attributes);
      }
    }
    $mappings = $mapping->getObjectsByProperties();
    if (!empty($mappings)) {
      // There are mappings for this origin. We have to see if we keep them or delete them if they are covered by the new one.
      foreach ($mappings as $old_mapping) {
        // If it exists for this format and source or this one is more generic we delete the old one.
        // Except in the case of PREPROCESS mappings, which we allow to coexist with others (but not with other PREPROCESS mappings).
        if (($old_mapping->get('action') != 'PREPROCESS' && $action != 'PREPROCESS') || ($old_mapping->get('action') == 'PREPROCESS' && $action == 'PREPROCESS')) {
          $replace_format = (!$format_code || ($old_mapping->get('format') && $old_mapping->get('format')->get('code') === $format_code));
          $replace_source = (!$source_code || ($old_mapping->get('source') && $old_mapping->get('source')->get('code') === $source_code));
          $replace_type = (!$type_name || ($old_mapping->get('type') && $old_mapping->get('type')->get('name') === $type_name));
          if  ($replace_format && $replace_source && $replace_type) {
            // If the attributes are the same.
            if (!$attributes && !$old_mapping->get('attributes_origin')) {
              $old_mapping->delete();
            }
            elseif ($old_mapping->get('attributes_origin')) {
              sort($attributes);
              sort($old_mapping->get('attributes_origin'));
              if (IdsKsObject::equal($old_mapping->get('attributes_origin'), $attributes)) {
                $old_mapping->delete();
              }
            }
          }
        }
      }
    }
    if ($format_code) {
      $format = new IdsKsFormat();
      $format->set('code', $format_code);
      $format->load();
      if ($format->getId()) {
        $mapping->set('format', $format);
      }
    }
    if ($source_code) {
      $source = new IdsKsSource();
      $source->set('code', $source_code);
      $source->load();
      if ($source->getId()) {
        $mapping->set('source', $source);
      }
    }
    if ($type_name) {
      $type = new IdsKsType();
      $type->set('name', $type_name);
      $type->load();
      if ($type->getId()) {
        $mapping->set('type', $type);
      }
    }
    if ($relationship_id) {
      $relationship = new IdsKsRelationship();
      $relationship->load($relationship_id);
      if ($relationship->get('type')) {
        $mapping->set('relationship', $relationship);
      }
      $mapping->set('action', 'RELATED_ITEM');
    }
    elseif (in_array($action, array_keys(idskshub_mapping_actions()))) {
      $mapping->set('action', $action);
    }
    $mapping->set('path_destination', $path_destination);
    $mapping->save();
    return $mapping;
  }

  public static function getMapping($path_origin, $attributes=array(), $format=NULL, $source=NULL, $type=NULL, $action='', $exclude_action='') {
    $mapping_found = new IdsKsMapping;
    $mapping = new IdsKsMapping;
    $mapping->set('path_origin', $path_origin);
    if ($action) {
      $mapping->set('action', $action);
    }
    $mappings = $mapping->getObjectsByProperties();
    if (!$mappings) { // mapping not found. check if a mapping exists with a wildcard.
      $tag = array_pop(explode('/', $path_origin));
      $mapping->set('path_origin', "*/$tag");
      $mappings = $mapping->getObjectsByProperties();
    }
    if ($mappings) {
      foreach ($mappings as $mapping) {
        $format_matches = (!$mapping->get('format') || $mapping->get('format')->getId() == $format->getId());
        $source_matches = (!$mapping->get('source') || $mapping->get('source')->getId() == $source->getId());
        $type_matches = (!$mapping->get('type') || $mapping->get('type')->getId() == $type->getId());
        $action_matches = ($mapping->get('action') != $exclude_action);
        if ($format_matches && $source_matches && $type_matches && $action_matches) {
          if (!$mapping->get('attributes_origin') || !(array_udiff($mapping->get('attributes_origin'), $attributes, 'idskshub_diff'))) { // the attributes of the mapping are included in the passed attributes.
            $mapping_found = $mapping; // the destination is found. we stop looking.
            break;
          }
        }
      }
    }
    return $mapping_found;
  }

  public static function getMappings($path_origin, $attributes=array(), $format=NULL, $source=NULL, $type=NULL, $action='', $exclude_action='') {
    $mappings_found = array();
    $mapping = new IdsKsMapping;
    $mapping->set('path_origin', $path_origin);
    if ($action) {
      $mapping->set('action', $action);
    }
    $mappings_field = $mapping->getObjectsByProperties();
    $tag = array_pop(explode('/', $path_origin)); // check if a mapping exists with a wildcard.
    $mapping->set('path_origin', "*/$tag");
    $mappings_wildcard = $mapping->getObjectsByProperties();
    $mappings = array_merge($mappings_field, $mappings_wildcard);
    if ($mappings) {
      foreach ($mappings as $mapping) {
        $format_matches = (!$mapping->get('format') || $mapping->get('format')->getId() == $format->getId());
        $source_matches = (!$mapping->get('source') || $mapping->get('source')->getId() == $source->getId());
        $type_matches = (!$mapping->get('type') || $mapping->get('type')->getId() == $type->getId());
        $action_matches = ($mapping->get('action') != $exclude_action);
        if ($format_matches && $source_matches && $type_matches && $action_matches) {
          if (!$mapping->get('attributes_origin') || !(array_udiff($mapping->get('attributes_origin'), $attributes, 'idskshub_diff'))) { // the attributes of the mapping are included in the passed attributes.
            $mappings_found[] = $mapping;
          }
        }
      }
    }
    return $mappings_found;
  }

  public static function isIgnore($path_origin, $attributes=array(), $format=NULL, $source=NULL, $type=NULL) {
    $mapping = IdsKsMapping::getMapping($path_origin, $attributes, $format, $source, $type, 'IGNORE');
    return !IdsKsObject::isEmpty($mapping);
  }

  public static function isPreprocess($path_origin, $attributes=array(), $format=NULL, $source=NULL, $type=NULL) {
    $mapping = IdsKsMapping::getMapping($path_origin, $attributes, $format, $source, $type, 'PREPROCESS');
    return $mapping->get('path_destination');
  }

  public static function getMappingDestination($path_origin, $attributes = array(), $format = NULL, $source = NULL, $type = NULL) {
    $mapped = $path_origin;
    $mapping = IdsKsMapping::getMapping($path_origin, $attributes, $format, $source, $type);
    if (!IdsKsObject::isEmpty($mapping)) {
      $mapped = $mapping->get('path_destination');
    }
    return $mapped;
  }

  public function getLabel() {
    $attributes_str = '';
    $format = ($this->get('format')) ? $this->get('format')->get('code') : '*';
    $source = ($this->get('source')) ? $this->get('source')->get('code') : '*';
    $type = ($this->get('type')) ? $this->get('type')->get('name') : '*';
    $path_origin = ($this->get('path_origin')) ? $this->get('path_origin') : '*';
    $path_destination = ($this->get('path_destination')) ? $this->get('path_destination') : '*';
    if ($this->get('attributes_origin')) {
      $attributes = array();
      foreach ($this->get('attributes_origin') as $attribute) {
        $attributes[] = $attribute->get('name') . '=' . $attribute->get('value');
      }
      if ($attributes) {
        $attributes_str = ' [' . implode(',', $attributes) . ']';
      }
    }
    return $format . ':' . $source . ':' . $type . ':' . $path_origin  . $attributes_str . ' --> ' . $path_destination;
  }

} // class IdsKsMapping
