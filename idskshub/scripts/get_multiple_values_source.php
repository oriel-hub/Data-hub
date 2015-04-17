<?php

// Get items with more than one value for $field_name.

$vl = new IdsKsVersionsLiteral;
$vl->path = 'object_id';
$vls = $vl->getObjectsByProperties();

$multiple_values = array();
foreach ($vls as $vlit) {
  $values = array();
  foreach ($vlit->literals as $lit) {
    $values[$lit->source->code][] = $lit->value;
  }
  if (isset($values['eldis']) && count($values['eldis']) > 1) {
    $multiple_values['eldis'][] = $vlit->getId();
  }
  if (isset($values['bridge']) && count($values['bridge']) > 1) {
    $multiple_values['bridge'][] = $vlit->getId();
  }
}

$multiple_items = array();
foreach ($multiple_values as $source => $versions) {
  $query = new EntityFieldQuery();
  $items = $query
      ->entityCondition('entity_type', 'ids_ks_item')
      ->fieldCondition('field_item_versions_literals', 'target_id', $versions, 'IN')
      ->execute();
  $items_ids = array_keys($items['ids_ks_item']);
  $multiple_items[$source] = $items_ids;
}

print_r($multiple_items);









