<?php
// $Id$

/**
 * @file
 * IdsKsObject class.
 * This class provides persistence to IDS objects by the use of Drupal's entities.
 * The implementation of this class could be changed in order to provide persistence in other ways.
 *
 */

// Ver funciones en http://drupalcontrib.org/api/drupal/contributions%21entity%21entity.module/7

class IdsKsObject {

  // Given an object, its type and the name of a field/property, returns its value.
  public static function factory($entity_type) {
    $classname = self::getClassName($entity_type);
    return new $classname();
  }

  public static function getClassName($entity_type) {
    $classname = ''; 
    foreach (explode('_', $entity_type) as $part) {
      $classname .= ucfirst($part);
    }
    if (!$classname) {
      $classname = 'IdsKsObject';
    }
    return $classname;
  }

  public static function equal($object1, $object2) {
    if (is_array($object1) && is_array($object2)) {
      $are_equal = TRUE;
      while ($are_equal && list($key,$val1) = each($object1)) {
        $are_equal = (isset($object2[$key]) && IdsKsObject::equal($object1[$key], $object2[$key]));
      }
    }
    elseif (is_a($object1, 'IdsKsObject') && is_a($object2, 'IdsKsObject')) {
      if (get_class($object1) == get_class($object1)) {
        $properties1 = array_diff(array_keys(get_object_vars($object1)), array('id'));
        $properties2 = array_diff(array_keys(get_object_vars($object2)), array('id'));
        if ($properties1 == $properties2) {
          $are_equal = TRUE;
          while ($are_equal && list($key,$property) = each($properties1)) {
            $value1 = $object1->get($property);
            $value2 = $object2->get($property);
            $are_equal = IdsKsObject::equal($value1, $value2);
          }
        }
        else {
          $are_equal = FALSE;
        }
      }
      else {
        $are_equal = FALSE;
      }
    }
    else {
      $are_equal = ($object1 == $object2);
    }
    return $are_equal;
  }

  public static function isEmpty($obj) {
    $is_empty = TRUE;
    if (!empty($obj)) {
      if (!is_object($obj) && !is_array($obj)) {
        $is_empty = FALSE;
      }
      else {
        foreach ($obj as $property) {
          if (!$is_empty = $is_empty && self::isEmpty($property)) {
            break;
          }
        }
      }
    }
    return $is_empty;
  }

  // Populates the current object with data from an existing entity (identified by its id or other keys that are already set in the object).
  public function load($id = 0) {
    if (is_numeric($id)) {
      if ($id) {
        $this->setId($id);
      }
      if ($this->getId()) {
        $this->populate();
      }
      else { // fills in the blank properties with data from the entities. makes sense only if the properties with values are keys.
        if ($new_object = $this->getSingleObjectByProperties()) {
          foreach ($new_object as $property => $value) {
            $this->{$property} = $value;
          }
        }
      }
    }
  }

  protected function addConditionsArray($query, $elements, $field_name) {
    foreach ($elements as $element) {
      if (is_a($element, 'IdsKsObject')) {
        $objects = $element->getObjectsByProperties();
        foreach ($objects as $object) {
          if ($object->getId()) {
            $query->fieldCondition($field_name, 'target_id', $object->getId(), '=');
          }
        }
      }
      elseif (is_array($element)) { // it should have been serialized when saving, so we compare to the serialized value.
        $query->fieldCondition($field_name, 'value', serialize($element), '=');
      }
      else {
        $query->fieldCondition($field_name, 'value', $element, '=');
      }
    }
  }

  protected function addConditionsObject($query, $object, $field_name) {
    if (!$id_value = $object->getId()) {
      $results = $object->getObjectsByProperties();
      $values_ids = array();
      foreach ($results as $result) {
        if ($result->getId()) {
          $values_ids[] = $result->getId();
        }
      }
      if (!empty($values_ids)) {
        $query->fieldCondition($field_name, 'target_id', $values_ids, 'IN');
      }
    } else {
      $query->fieldCondition($field_name, 'target_id', $id_value, '=');
    }
  }

  public function getEntitiesByProperties() {
    $entities = array();
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', $this->getEntityType());
    $query->entityCondition('bundle', $this->getBundleType());
    foreach ($this as $property => $value) {
      if (($property != 'id') && isset($value)) {
        if (is_array($value)) {
          self::addConditionsArray($query, $value, $this->getFieldName($property));
        }
        elseif (is_a($value, 'IdsKsObject')) {
          self::addConditionsObject($query, $value, $this->getFieldName($property));
        }
        else {
          $query->fieldCondition($this->getFieldName($property), 'value', $value, '=');
        }
      }
    }
    $results = $query->execute();
    if (isset($results[$this->getEntityType()])) {
      $entities = $results[$this->getEntityType()];
    }
    return $entities;
  }

  public function isSaved() {
    return (entity_load_single($this->getEntityType(), $this->getId())) ? TRUE : FALSE;
  }

  public function getObjectsByProperties() {
    $objects = array();
    $entities = $this->getEntitiesByProperties();
    $ids = array_keys($entities);
    foreach ($ids as $id) {
      $object = self::factory($this->getEntityType());
      $object->setId($id);
      $object->load();
      $objects[] = $object;
    }
    return $objects;
  }

  public function getSingleEntityByProperties() {
    $entity = NULL;
    $entities = $this->getEntitiesByProperties();
    if (!empty($entities)) {
      $entity = array_pop($entities);
    }
    return $entity;
  }

  public function getSingleObjectByProperties() {
    $object = NULL;
    $objects = $this->getObjectsByProperties();
    if (isset($objects[0])) {
      $object = $objects[0];
    }
    return $object;
  }

  public function copy($object) {
    foreach ($this as $property => $value) {
      if (isset($object->{$property})) {
        $this->{$property} = $object->{$property};
      }
    }
  }

  // Save the current object to an entity (updates it based on the ID, or creates it if it doesn't exist).
  // $recursive = TRUE creates all the referenced entities with their corresponding types, but we don't want to do it all the time.
  public function save($recursive = FALSE) {
    $entity = NULL;
    if ($this->getId()) {
      $entity = $this->getEntity();
    }
    if (!$entity) { // The entity was not retrieved. Create a new empty entity.
      $entity = $this->createEmptyEntity();
    }
    if ($entity) {
      $wrapper = entity_metadata_wrapper($entity->entityType(), $entity);
      foreach ($this as $property => $value) {
        if (is_array($value)) {
          $field_value = self::saveMultipleValues($value, $recursive);
        }
        elseif (is_object($value)) {
          if (is_subclass_of(get_class($value), 'IdsKsObject')) {
            if ($recursive) {
              $field_value = $value->save($recursive);
            }
            else {
              if ($id = $value->getId()) {
                $field_value = $id;
              }
            }
          }
          else { // if it corresponds to an entity reference, we create a new entity with the values of the object.
            if ($recursive) {
              $field_name = $this->getFieldName($property);
              $field_info = field_info_field($field_name);
              if (isset($field_info['settings']['target_type'])) {
                $property_type = $field_info['settings']['target_type'];
                $new_object = IdsKsObject::factory($property_type);
                $new_object->copy($value);
                $field_value = $new_object->save($recursive);
              }
            }
          }
        }
        else {
          $field_value = $value;
        }
        $field_name = $this->getFieldName($property);
        if (isset($wrapper->{$field_name}) && isset($field_value)) { // We only update those fields that have some value assigned.
          $wrapper->{$field_name} = $field_value;
        }
      }
      $wrapper->save();
    }
    if (is_a($entity, 'Entity')) {
      $id = $entity->identifier();
    }
    else {
      $id = 0;
    }
    $this->setId($id);
    return $id;
  }

  // Save array.
  protected function saveMultipleValues($elements, $recursive) {
    $values = array();
    foreach ($elements as $element) {
      if (is_a($element, 'IdsKsObject')) {
        if ($recursive) {
          $element->save($recursive);
        }
        if ($id = $element->getID()) {
          $values[] = $element->getID();
        }
      }
      elseif (is_array($element)) { // not possible with entities, so we serialize it.
        $values[] = serialize($element);
      }
      else {
        $values[] = $element;
      }
    }
    return $values;
  }

  public static function touchMultiple($array_ids) {
    $entity_type = self::getEntityType();
    $entities = entity_load($entity_type, $array_ids);
    foreach ($entities as $entity) {
      $wrapper = entity_metadata_wrapper($entity_type, $entity);
      $wrapper->save();    
    }
  }

  // Delete the entity corresponding to the current object, based on the ID.
  public function delete() {
    if ($this->getId()) {
      return entity_delete($this->getEntityType(), $this->getId());
    }
    else {
      return FALSE;
    }
  }

  // Delete multiple entities
  public static function deleteMultiple($array_ids) {
    return entity_delete_multiple(self::getEntityType(), $array_ids); 
  }

  // delete all objects of one class.
  public function deleteAll($classname) {
    $entity_type = 'ids_ks_' . self::getTypeFromClass($classname);
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', $entity_type, '=');
    $result = $query->execute();
    if (isset($result[$entity_type])) {
      $entities = entity_load($entity_type, array_keys($result[$entity_type]));
      entity_delete_multiple($entity_type, array_keys($entities));
    }
  }

  // Get the id of the current object and the corresponding entity. It's 0 if it still doesn't have an entity associated to it.
  public function getId() {
    if (isset($this->id)) {
      $id = $this->id;
    }
    else {
      $id = 0;
    }
    return $id;
  }

  // Set the id of the current object.
  public function setId($id) {
    $this->id = $id;
  }

  // Get the short format of the type based on the class of the current object.
  public function getObjType() {
    $type =  self::getTypeFromClass(get_called_class());
    return $type;
  }

  // get the short type from a classname.
  protected static function getTypeFromClass($classname) {
    return strtolower(trim(preg_replace('/([A-Z]{1})/', '_\1', preg_replace('/IdsKs/', '', $classname)), '_'));
  }

  // Get the entity name corresponding to the class of the current object.
  public function getEntityType() {
    $entity_type = 'ids_ks_' . self::getObjType();
    return $entity_type;
  }

  // Get the bundle name corresponding to the class of the current object.
  public function getBundleType() {
    $bundle_type = 'ids_ks_' . self::getObjType();
    return $bundle_type;
  }

  // Get the name of an entity field, corresponding to a property of the current object.
  public function getFieldName($property) {
//    $field_name = 'field_' . self::getObjType() . '_' . $property;
    $entity_type = self::getEntityType();
    $bundle_name = self::getBundleType();
    $field_name = 'field_' . self::getObjType() . '_' . $property;
    if (!$field_exists = field_info_instance($entity_type, $field_name, $bundle_name)) {
      $field_name = 'field_' . $property;
      $field_exists = field_info_instance($entity_type, $field_name, $bundle_name);
    }
    if (!$field_exists) {
      $field_name = $property;
    }
    return $field_name;
  }

  // Create a new entity from the data of the current object.
  protected function createEmptyEntity() {
    $values = array();
    $entity_type = $this->getEntityType(); // TODO: check that these are valid entity and bundle types.
    $bundle_type = $this->getBundleType();
    $values['type'] = $bundle_type;
    $entity = entity_create($entity_type, $values);
    return $entity;
  }

  // Get the entity associated to the current object, based on ID.
  public function getEntity() {
    $entity = NULL;
    if ($this->getId()) {
      $entity = entity_load_single($this->getEntityType(), $this->getId());
    }
    return $entity;
  }

  public function set($property, $value) {
    $this->{$property} = $value;
  }

  public function add($property, $value) {
    $this->{$property}[] = $value;
  }

  public function get($property, $wrapper=NULL) {
    if (!isset($this->{$property}) && $this->getId() && ($entity = $this->getEntity())) {
      if (!isset($wrapper)) {
        $wrapper = entity_metadata_wrapper($this->getEntityType(), $entity, array('bundle' => $this->getBundleType()));
      }
      $field_name = $this->getFieldName($property);
      if (isset($wrapper->{$field_name})) {
        $field_value = $wrapper->{$field_name}->value();
        if (is_array($field_value)) {
          $this->{$property} = $this->populateMultipleValues($field_value);
        }
        elseif (is_a($field_value, 'Entity')) {
          $this->{$property} = self::factory($field_value->bundle()); // Create a new IdsKsObject corresponding to this type.
          $this->{$property}->setId($field_value->identifier());
        }
        else {
          $this->{$property} = $field_value;
        }
      }
    }
    return $this->{$property};
  }

  protected function populateMultipleValues($elements) {
    $values = array();
    foreach ($elements as $element) {
      if (is_a($element, 'Entity')) {
        $new_value = self::factory($element->bundle());
        $new_value->setId($element->identifier());
        $values[] = $new_value;
      }
      else {
        $value = idskshub_unserialize($element);
        $values[] = $value;
      }
    }
    return $values;
  }

  public function populate() {
    if ($this->getId() && ($entity = $this->getEntity())) {
      $wrapper = entity_metadata_wrapper($this->getEntityType(), $entity, array('bundle' => $this->getBundleType()));
      $properties = array_keys(get_object_vars($this));
      foreach ($properties as $property) {
        $this->get($property, $wrapper);
      }
    }
  }

  // return an array of the objects retrieved using the passed values as conditions.
  public static function getArrayObjects($class, $properties) {
    $values = array();
    $conditions = new $class;
    $objects = $conditions->getObjectsByProperties();
    if (!empty($objects)) {
      foreach ($objects as $object) {
        if ($id = $object->getId()) {
          $values[$id] = array();
          foreach ($properties as $property) {
            if (isset($object->{$property})) {
              $values[$id][$property] = $object->{$property};
            }
          }
        }
      }
    }
    return $values;
  }

  public static function markToIndex($ids_to_index) {
    $type = self::getEntityType();
    $entities = entity_load($type, $ids_to_index);
    foreach ($entities as $entity) {
      search_api_entity_update($entity, $type);
    }
  }

  /*
  Functions that retrieve entities' properties directly, in order to improve efficiency of some operations.
  */
  public static function getFieldValues($property, $filter_entities_ids = array()) {
    $values = array();
    $field_name = self::getFieldName($property);
    $table_name = 'field_data_' . $field_name;
    $value_name = $field_name . '_value';
    if (!db_field_exists($table_name, $value_name)) { // if not simple value, we see if it's an entity reference.
      $value_name = $field_name . '_target_id';
    }
    if (db_field_exists($table_name, $value_name)) {
      $query = db_select($table_name, 'p')->fields('p', array('entity_id', $value_name));
      if ($filter_entities_ids) {
        $query->condition('entity_id', $filter_entities_ids, 'IN');
      }
      $results = $query->execute();
      while ($value = $results->fetchObject()) {
        $values[$value->entity_id][] = $value->{$value_name};
      }
    }
    return $values;
  }

  // Get entity IDs by properties.
  public static function getIds($filters = array()) {
    $ids = array();
    $type = self::getEntityType();
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', $type, '=');
    $filters_exist = TRUE;
    foreach ($filters as $field => $value) {
      if (is_null($value) || is_array($value) && empty($value)) {
        $filters_exist = FALSE;
        break;
      }
      else {
        if (is_array($value)) {
          $op = 'IN';
        }
        else {
          $op = '=';
        }
        $field_name = self::getFieldName($field);
        $table_name = 'field_data_' . $field_name;
        $value_name = $field_name . '_value';
        if (db_field_exists($table_name, $value_name)) {
          $query->fieldCondition($field_name, 'value', $value, $op);
        }
        else {
          $value_name = $field_name . '_target_id';
          if (db_field_exists($table_name, $value_name)) {
            $query->fieldCondition($field_name, 'target_id', $value, $op);
          }
          else {
            $filters_exist = FALSE;
            break;
          }
        }
      }
    }
    if ($filters_exist) {
      $results = $query->execute();
      if (isset($results[$type])) {
        $ids = array_unique(array_keys($results[$type]));
      }
    }
    return $ids;
  }

  public static function getChildrenIds($parent_id = 0, $property) {
    $result = array();
    $children = IdsKsItem::getFieldValues($property, array($parent_id));
    if (isset($children[$parent_id])) {
      $result = $children[$parent_id];
    }
    return $result;
  }

} // IdsKsObject class.

// Additional functions

// Unserialize.
function idskshub_unserialize($element) {
  $result = @unserialize($element);
  if ($result || ($element == serialize(false))) {
    return $result;
  }
  else {
    return $element;
  }
}





