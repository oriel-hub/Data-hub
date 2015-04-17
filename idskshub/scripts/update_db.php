<?php

  // CAMBIAR IDS PARA VERSION LIVE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  $types_relationships = array
   (
   // 61 => 1, // Regions
      62 => 2, // Countries
   // 66 => 3, // Document Types
   // 63 => 4, // Themes
   // 60 => 5, // Organisations
   // 64 => 7, // Subjects
    );
  foreach ($types_relationships as $type_id => $relationship_id) {
    echo "type_id: $type_id\n";
    echo "relationship_id: $relationship_id\n";
    $relationship = new IdsKsRelationship();
    $relationship->load($relationship_id);
    $categories_ids = IdsKsItem::getParentsIds(array('type' => $type_id));
    foreach ($categories_ids as $category_id) {
      $related_items_category = IdsKsRelatedItem::getParentsIds(array('referred_item' => $category_id));
      $category = new IdsKsItem();
      $category->load($category_id);
      echo "category: $category->display_title\n";
      $items_ids = IdsKsItem::getParentsIds(array('subitems' => $category_id));
      foreach ($items_ids as $item_id) {
        $related_items_item = IdsKsItem::getChildrenIds($item_id, 'related_items');
        if(empty($related_items_item) || !array_intersect($related_items_item, $related_items_category)) {
          $item = new IdsKsItem();
          $item->load($item_id);
          if (!empty($item->track_changes)) {
            $item_sources_ids = array();
            foreach ($item->track_changes as $change) {
              if (isset($change->source)) {
                $source = $change->source;
                if (!in_array($source->getId(), $item_sources_ids)) {
                  $item_sources_ids[] =  $source->getId();
                  if (!IdsKsRelationship::isEmpty($relationship)) {
                    if (!$category->isEmptyItem()) {
                      $related_item = new IdsKsRelatedItem();
                      $related_item->relationship = $relationship;
                      $related_item->source = $source;
                      $related_item->referred_item = $category;
                      $related_item->save();
                      $item->related_items[] = $related_item;
                      $item->save();
                      unset($related_item);
                      echo "saving: $source->name: $item_id -> $category_id\n";
                    }
                    else {
                      echo "empty $category_id\n";
                    }
                  }
                  else {
                    echo "empty relationship\n";
                  }
                }
              }
              else {
                echo "no source\n";
              }
            }
          }
          else {
            echo "no track_changes $category_id\n";
          }
          unset($item);
        }
      }
      unset($category);
    }
  }


