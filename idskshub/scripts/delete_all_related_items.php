<?php

// Get all items with relationships.
$query = new EntityFieldQuery();
$items = $query
		->entityCondition('entity_type', 'ids_ks_item')
		->fieldCondition('field_item_related_items', 'target_id', null, 'IS NOT NULL')
		->execute();
$items_ids = array_keys($items['ids_ks_item']);

foreach ($items_ids as $item_id) {
  echo "deleting from $item_id\n";
  $item = new IdsKsItem();
  $item->load($item_id);
  $item->related_items = array();
  $item->save();
  unset($item);
}

//IdsKsObject::deleteAll('IdsKsRelatedItem');

