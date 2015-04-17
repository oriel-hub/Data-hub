<?php

$source_id = 21;
$rel_items = IdsKsRelatedItem::getParentsIds(array('source' => $source_id));

$query = new EntityFieldQuery();
$items = $query
		->entityCondition('entity_type', 'ids_ks_item')
		->fieldCondition('field_item_related_items', 'target_id', $rel_items, 'IN')
		->execute();
$items_ids = array_keys($items['ids_ks_item']);

foreach ($items_ids as $item_id) {
  $all_rel_items = IdsKsItem::getFieldValues('related_items', array($item_id));
  $rel_item_ids = IdsKsRelatedItem::getFieldValues('referred_item', $all_rel_items);
  $array_related_items[$item_id] = array();
  $rel_item_ids_added = array();
  foreach ($rel_item_ids as $rel_item_id => $array_rel_item_ids) {
    $referred_item= $array_rel_item_ids[0];
    if (!in_array($rel_item_id, $rel_items)) {
      $array_related_items[$item_id][] = $rel_item_id;
    }
    elseif (!in_array($referred_item, $rel_item_ids_added)) {
      $array_related_items[$item_id][] = $rel_item_id;
      $rel_item_ids_added[] = $referred_item;
    }    
    else {
      echo "delete $rel_item_id from $item_id\n";
      entity_delete('ids_ks_related_item', $rel_item_id);
    }
  }
}

$entities_items = entity_load('ids_ks_item', $items_ids);
foreach ($entities_items as $entity_id => $entity) {
  $wrapper = entity_metadata_wrapper('ids_ks_item', $entity);
  $wrapper->{'field_item_related_items'} =  $array_related_items[$entity_id];
  echo "saving $entity_id\n";
  $wrapper->save();
}

