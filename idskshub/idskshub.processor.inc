<?php
// $Id$

/**
 * @file
 *   IdsKsProcessor class.
 *
 */

class IdsKsProcessor {

  public static function batchProcess($items, $item_path, $source, $feed, $format, $type_name, $total_elements, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $index_imported, $force_import, $root_file_attributes) {
    $message = '';
    idskshub_set('import_active', TRUE);
    idskshub_set('import_time', date("Y-m-d H:i:s"));
    idskshub_set('index_imported', $index_imported['index']);
    if (($total_elements > IDSKS_NUM_ELEMENTS_INTERACTIVE) && ($mode == 'default')) {
      $mode = 'background';
      $message = t('Importing continues in background. You can leave this page.') . '<br>';
    }
    $num_skipped = 0;
    $xml_items = array();
    foreach ($items as $item) { // items have to be serialized to queue the process and SimpleXMLElements are not serializable.
      $xml_items[] = $item->asXML();
    }
    $num_items = count($xml_items);
    $init_message = '';
    if ($num_items) {
      $imp_items = ($num_items > 1) ? 'items' : 'item';
      $init_message = t('Importing @num_items @imp_items... ', array('@num_items' => $num_items, '@imp_items' => $imp_items));
    }
    $batch = array(
      'operations' => array(
        array('IdsKsProcessor::batchProcessItems', array($xml_items, $item_path, $source, $feed, $format, $type_name, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $force_import, $root_file_attributes)),
      ),  
      //'finished' => 'IdsKsProcessor::endProcessItems', not working?.
      'title' => t('Importing from ') . $source->get('name'),
      'init_message' => $init_message,
      'progress_message' => '',
      'error_message' => t('Error in the importing process.'),
      'file' => drupal_get_path('module', 'idskhub') . '/idskhub.processor.inc',
    );
    batch_set($batch);
    if ($mode == 'background') {
      $message .= t('To follow the import progress, please go to the <a href="@imported_url">imported items page</a>.', array('@imported_url' => url('imported-items')));
      background_batch_process_batch('imported-items');
    }
    else {
      batch_process('imported-items');
    }
    drupal_set_message($message);
  }

  //Process in batch.
//  public static function batchProcessItems($xml_items, $item_path, $source, $feed, $format, $type_name, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $force_import, $root_file_attributes, &$context) {
//    $items = array_map('simplexml_load_string', $xml_items);    
  public static function batchProcessItems($items, $item_path, $source, $feed, $format, $type_name, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $force_import, $root_file_attributes, &$context) {
    if (!isset($context['sandbox']['progress'])) {
      $context['sandbox']['progress'] = 0;
      $context['sandbox']['max'] = count($items);
      $context['sandbox']['num_item'] = 0;
      $context['sandbox']['num_skipped_items'] = 0;
      $context['sandbox']['num_imported_items'] = 0;
      $context['sandbox']['num_updated_items'] = 0;
      $context['sandbox']['num_unchanged_items'] = 0;
    }
    $num_item = $context['sandbox']['num_item'];
    if (isset($items[$num_item])) {
      $item = $items[$num_item];
      $context['sandbox']['num_item']++;
      if (list($display, $item_id, $item_type) = self::processSingleItem($item, $item_path, $source, $feed, $format, $type_name, $semantic, $existing_items, $update_mode, $allow_multiple, $force_import, $root_file_attributes)) {
        $context['items_imported'][$item_type][] = $item_id; // not used?
        $context['results'][$item_type][] = $item_id; // not used?
        $context['message'] = t('@display @item_type @item_id', array('@display' => ucfirst($display), '@item_type' => $item_type, '@item_id' => $item_id));
        $context['sandbox']['num_'.$display.'_items']++;
      }
      else {
        $context['sandbox']['num_skipped_items']++;
      }
      $context['message'] .= '<br>';
      $context['message'] .= t('Processed ');
      $context['message'] .= $num_item + 1 . ' out of ' . $context['sandbox']['max'];
      $context['message'] .= '<br><br>';
      if ($context['sandbox']['num_skipped_items']) {
        $context['message'] .= $context['sandbox']['num_skipped_items'] . t(' items skipped.');
        $context['message'] .= '<br>';
      }
      if ($context['sandbox']['num_imported_items']) {
        $context['message'] .= $context['sandbox']['num_imported_items'] . t(' new items imported.');
        $context['message'] .= '<br>';
      }
      if ($context['sandbox']['num_updated_items']) {
        $context['message'] .= $context['sandbox']['num_updated_items'] . t(' existing items updated.');
        $context['message'] .= '<br>';
      }
      if ($context['sandbox']['num_unchanged_items']) {
        $context['message'] .= $context['sandbox']['num_unchanged_items'] . t(' existing items unchanged.');
        $context['message'] .= '<br>';
      }
      idskshub_set('import_progress', $context['message']);
      $context['sandbox']['progress']++;
      if ($context['sandbox']['max'] > 0) {
        $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
      }
      else {
        $context['finished'] = 1;
      }      
    }
    else {
      $context['finished'] = 1;
    }
    if ($context['finished'] == 1) {
      idskshub_set('import_active', FALSE);
      idskshub_set('import_time', date("Y-m-d H:i:s"));
      idskshub_set('import_error_message', '');
      idskshub_set('import_progress', '');
      IdsKsProcessor::endProcessItems($context);
    }
  }

  // Process each item and create/update the corresponding entity.
  public function processSingleItem($serialized_item, $item_path, $source, $feed, $format, $type_name, $semantic, $existing_items, $update_mode, $allow_multiple, $force_import, $root_file_attributes) {
    $okhub_source = new IdsKsSource;
    $okhub_source->set('code', IDSKS_OKHUB_SOURCE_CODE);
    $okhub_source->load();
    $item_id = 0;
    $related_items = array();
    $hash = IdsKsTrackChanges::newHash($serialized_item);
    if ($force_import['force'] || !IdsKsTrackChanges::hashExists($hash)) {
      $xml_item = simplexml_load_string($serialized_item);
      $new_item = new IdsKsItem();
      $new_item->setItemType($source, $feed, $type_name); // Assign type before populating (to use in relationships, mappings, etc). The type should be known now.
      $new_item->populateItemXML($xml_item, $item_path, $source, $format);
      if (!empty($root_file_attributes)) {
        $new_item->addRootAttributes($root_file_attributes, $source);
      }
      // We set the display_title so we can use it to identify existing items.
      $new_item->setDisplayTitle();
      // if the item does not exist, we save the newly created item. If it exists, we update the values for this source.
      $items_ids = $new_item->getDuplicatesBySource($source);
      if ($found_by_source = !empty($items_ids)) {
        $update_existing = TRUE;
      }
      else {
        $items_ids = $new_item->getDuplicatesByType();
        $update_existing = $new_item->get('type')->get('update_existing');
      }
      if (empty($items_ids)) { // The item is new.
        $item = $new_item;
        $display = 'imported';
      }
      else {
        if ($update_existing) { // Update existing item.
          if (count($items_ids) > 1) {
            //IdsKsItem::mergeMultipleItemsById($items_ids);
            $merge_candidates = idskshub_get('merge_candidates', array());
            $merge_candidates[] = $items_ids;
            idskshub_set('merge_candidates', $merge_candidates); // TODO: Add administrative interface to show and choose to merge.
            trigger_error(t('Potentially duplicated items found. Please consider merging them: @item_ids.', array('@item_ids' => implode(',', $items_ids))));
          }
          $item_id = array_shift($items_ids); // the first id will be the item's, if there is only one, or the id of the merged item if mergeMultipleItemsById() is enabled.
          if ($item_id) {
            $item = new IdsKsItem();
            $item->load($item_id);
            if ($existing_items == 'update_existing' || (!$found_by_source && !$item->hasContentFromSource($source->getId()))) {
              $item->updateSourceValues($new_item, $source, $update_mode, $allow_multiple);
              $display = 'updated';
            }
            else {
              $display = 'unchanged';
            }
          }
        }
        else { // Create new item and add "same as" links to existing items.
          $item = $new_item;
          $display = 'imported and linked to existing items';
          // Get "same as" relationship based on the item's type.
          $same_as_rel = new IdsKsRelationship;
          if ($item->get('type')->get('name')) {
            $same_as_rel->set('code', 'same_as_' . strtolower($item->get('type')->get('name')));
            $same_as_rel->load();
            if ($same_as_rel->getId()) {
              foreach ($items_ids as $item_id) {
                $rel_item = new IdsKsItem;
                $rel_item->load($item_id);
                $related_items[] = $rel_item;
                $new_rel_item = new IdsKsRelatedItem;
                $new_rel_item->set('referred_item', $rel_item);
                $new_rel_item->set('relationship', $same_as_rel);
                $new_rel_item->set('source', $okhub_source);
                $new_rel_item->save();
                $item->add('related_items', $new_rel_item);
              }
            }
          }
        }
      }
      $item->saveSubitems();
      $item->saveRelatedItems();
      $item->addChanges($source, $serialized_item, $hash);
      $item->addSemanticData($semantic);
      $item->saveItem();
      $item_id = $item->getId();      
      if ($item_id && !$update_existing && !empty($related_items) && $same_as_rel->getId()) { // In this case there are items and we have to create "same as" links from them.
        foreach ($related_items as $rel_item) {
          $new_rel_item = new IdsKsRelatedItem;
          $new_rel_item->set('referred_item', $item);
          $new_rel_item->set('relationship', $same_as_rel);
          $new_rel_item->set('source', $okhub_source);
          $new_rel_item->save();
          $rel_item->add('related_items', $new_rel_item);
          $rel_item->save();
        }
      }
    }
    if ($item_id) {
      return array($display, $item_id, $item->get('type')->get('name'));
    }
    else {
      return FALSE;
    }
  }

  public static function endProcessItems($context) {
    if ($context['sandbox']['num_item']) {
      // in $context['results'] is an array($item_type => array($item_id1, $item_id2, ...)) 
      $index_imported = idskshub_get('index_imported');
      if ($index_imported) {
        $items_ids = idskshub_flatten_array($context['results']);
        idskshub_index_selected($items_ids);
      }
    }
  }


} // IdsKsProcessor class.