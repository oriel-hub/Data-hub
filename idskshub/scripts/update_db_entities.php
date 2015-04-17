<?php

// CAMBIAR IDS PARA VERSION LIVE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$types_relationships = array
 (
 61 => 1, // Regions
 // 62 => 2, // Countries
 // 66 => 3, // Document Types
 // 63 => 4, // Themes
 // 60 => 5, // Organisations
 // 64 => 7, // Subjects
 );

$paths_related_items = array
 (
  61 => 'category_region_array', // Regions
  62 => 'country_focus_array', // Countries
  66 => 'document_type_array', // Document Types
  63 => 'category_theme_array', // Themes
  60 => 'publisher_array', // Organisations
  64 => 'category_subject_array', // Subjects
 );

foreach ($types_relationships as $type_id => $relationship_id) {
  echo "type_id: $type_id\n";
  echo "relationship_id: $relationship_id\n";
  $categories_ids = IdsKsItem::getParentsIds(array('type' => $type_id));
  foreach ($categories_ids as $category_id) {
    $category_wrapper = entity_metadata_wrapper('ids_ks_item', $category_id);
    if ($category_wrapper->field_item_display_title && $category_wrapper->field_item_display_title->value()) {
      echo "category: " . $category_wrapper->field_item_display_title->value() . " ($category_id)\n";
      if (!empty($category_wrapper->field_item_track_changes)) {
        $category_sources_ids = array();
        foreach ($category_wrapper->field_item_track_changes->value() as $category_change_entity) {
          $category_change_wrapper = entity_metadata_wrapper('ids_ks_track_changes', $category_change_entity);
          $category_source = $category_change_wrapper->field_track_changes_source->value();
          if ($category_source_id = $category_source->identifier()) {
            if (!in_array($category_source_id, $category_sources_ids)) {
              $category_sources_ids[] = $category_source_id;
            }
          }
        }
      }     
      $related_items_category = IdsKsRelatedItem::getParentsIds(array('referred_item' => $category_id));
      if ($items_ids = IdsKsItem::getParentsIds(array('subitems' => $category_id))) {
        $items_entities = entity_load('ids_ks_item', $items_ids);
        foreach ($items_ids as $item_id) {
          echo "updating item $item_id\n";
          $related_items_item = IdsKsItem::getChildrenIds($item_id, 'related_items');
          if(empty($related_items_item) || !array_intersect($related_items_item, $related_items_category)) {
            $item_wrapper = entity_metadata_wrapper('ids_ks_item', $items_entities[$item_id]);
            if (!empty($item_wrapper->field_item_track_changes)) {
              $item_sources_ids = array();
              foreach ($item_wrapper->field_item_track_changes->value() as $change_entity) {
                $change_wrapper = entity_metadata_wrapper('ids_ks_track_changes', $change_entity);
                $source = $change_wrapper->field_track_changes_source->value();
                if ($source_id = $source->identifier()) {
                  if (!in_array($source_id, $item_sources_ids)) {
                    $item_sources_ids[] = $source_id;
                  }
                }
              }
            }
            if ($sources_to_add = array_intersect($category_sources_ids, $item_sources_ids)) {
              foreach ($sources_to_add as $source_id) {
                $entity_related_item = entity_create('ids_ks_related_item', array('type' => 'ids_ks_related_item'));
                $related_item_wrapper = entity_metadata_wrapper('ids_ks_related_item', $entity_related_item);
                $related_item_wrapper->field_related_item_relationship = $relationship_id;
                $related_item_wrapper->field_related_item_source = $source_id;
                $related_item_wrapper->field_related_item_path = $paths_related_items[$type_id];
                $related_item_wrapper->field_related_item_referred_item = $category_id;
                $related_item_wrapper->save();
                if ($related_item_id = $related_item_wrapper->getIdentifier()) {
                  $item_wrapper->field_item_related_items[] = $related_item_id;
                  $item_wrapper->save();
                  echo "saving $related_item_id: $source_id: $item_id -> $category_id\n";
                }
              }
            }
          }
          else {
            echo "relationship exists: $item_id -> $category_id\n";
          }
        }
      }
      else {
        echo "no items for the category\n";
      }
    }
    else {
      echo "IdsKsItem::deleteItem($category);\n";
    }
  }
}

