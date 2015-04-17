<?php

$relationships_paths = array
 (
  // 1 => 'category_region_array', // Regions
  2 => 'country_focus_array', // Countries
  // 3 => 'document_type_array', // Document Types
  4 => 'category_theme_array', // Themes
  5 => 'publisher_array', // Organisations
  7 => 'category_subject_array', // Subjects
 );

foreach ($relationships_paths as $rel_id => $path) {
  $related_items_ids = IdsKsRelatedItem::getParentsIds(array('relationship' => $rel_id));
  $related_items_entities = entity_load('ids_ks_related_item', $related_items_ids);
  foreach ($related_items_entities as $entity_related_item) {
    $related_item_wrapper = entity_metadata_wrapper('ids_ks_related_item', $entity_related_item);
    $related_item_wrapper->field_related_item_path = $relationships_paths[$rel_id];
    echo "saving: " . $related_item_wrapper->getIdentifier() . "\n";
    $related_item_wrapper->save();    
  }
}