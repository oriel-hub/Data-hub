<?php

// Delete duplicated values in items from source $source_id.

$source_id = 21;
$lits = IdsKsLiteral::getParentsIds(array('source' => $source_id));

$query = new EntityFieldQuery();
$vls = $query
		->entityCondition('entity_type', 'ids_ks_versions_literal')
		->fieldCondition('field_versions_literal_literals', 'target_id', $lits, 'IN')
		->execute();
$vls_ids = array_keys($vls['ids_ks_versions_literal']);

foreach ($vls_ids as $vl_id) {
  $all_lits = IdsKsVersionsLiteral::getFieldValues('literals', array($vl_id));
  $values = IdsKsLiteral::getFieldValues('value', $all_lits);
  $array_literals[$vl_id] = array();
  $values_added = array();
  foreach ($values as $lit_id => $array_values) {
    $val = $array_values[0];
    if (!in_array($lit_id, $lits)) {
      $array_literals[$vl_id][] = $lit_id;
    }
    elseif (!in_array($val, $values_added)) {
      $array_literals[$vl_id][] = $lit_id;
      $values_added[] = $val;
    }    
    else {
      echo "delete $lit_id from $vl_id\n";
      entity_delete('ids_ks_literal', $lit_id);
    }
  }
}

$entities_vls = entity_load('ids_ks_versions_literal', $vls_ids);
foreach ($entities_vls as $entity_id => $entity) {
  $wrapper = entity_metadata_wrapper('ids_ks_versions_literal', $entity);
  $wrapper->{'field_versions_literal_literals'} =  $array_literals[$entity_id];
  echo "saving $entity_id\n";
  $wrapper->save();
}

