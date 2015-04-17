<?php
    $id_old = '6468';
    $id_new = '6429';
    $related_items_ids = IdsKsRelatedItem::getParentsIds(array('referred_item' => $id_old));
    echo "Changing $id_old by $id_new in rel. items:\n";
    print_r($related_items_ids);
    $related_items_entities = entity_load('ids_ks_related_item', $related_items_ids);
    foreach ($related_items_entities as $entity_related_item) {
      $related_item_wrapper = entity_metadata_wrapper('ids_ks_related_item', $entity_related_item);
      $related_item_wrapper->field_related_item_referred_item = $id_new;
      $related_item_wrapper->save();    
    }
?>