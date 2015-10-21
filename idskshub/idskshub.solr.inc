<?php
// $Id$

/**
 * @file
 *   IdsKsHub Solr functions.
 */

require_once('idskshub.solrfields.inc');

class IdsKsFilterNonTypedItems extends SearchApiAbstractAlterCallback {
  public function alterItems(array &$items) {
    $generic_type = IdsKsType::getIds(array('name' => IDSKS_GENERIC_TYPE_NAME));
    foreach ($items as $id => &$item) {
      $entity_lang = $item->search_api_language;
      if (isset($item->field_item_type[$entity_lang][0]['target_id'])) {
        $type_id = $item->field_item_type[$entity_lang][0]['target_id'];
        if (in_array($type_id, $generic_type)) {
          unset($items[$id]);
        }
      }
    }
  }
} // class IdsKsFilterNonTypedItems

/* --------------- Hooks ---------------- */

// Implements hook_search_api_alter_callback_info().
function idskshub_search_api_alter_callback_info() {
  $callbacks['ids_ks_non_typed_items'] = array(
    'name' => t('IDS KS Typed Items'),
    'description' => t('Indexes only typed items.'),
    'class' => 'IdsKsFilterNonTypedItems',
  );
  return $callbacks;
}

// Implements hook_search_api_solr_field_mapping_alter()
function idskshub_search_api_solr_field_mapping_alter(SearchApiIndex $index, array &$fields) {
  $facets_fields = idskshub_solr_facets_fields();
  if ($index->item_type == 'ids_ks_item') {
    foreach ($facets_fields as $old_name => $new_name) {
      $fields[$old_name] = $new_name;
    }
  }
}

// Make sure that a commit is performed after indexing a batch of items.
function idskshub_search_api_items_indexed(SearchApiIndex $index, array $item_ids) {
  $commit_url = IDSKS_SOLR_SERVER.'/solr/'.IDSKS_SOLR_CORE.'/update?commit=true';
  $commit = file_get_contents($commit_url);
  //debug($commit, 'Commit results');
}

// Implements hook_search_api_solr_documents_alter()
function idskshub_search_api_solr_documents_alter(array &$documents, SearchApiIndex $index, array $items) {
  $entity_type = $index->getEntityType();
  $fields_translatable = idskshub_get_solr_translatable();
  $fields_types = idskshub_solr_fields_types(); // TODO: Change to use idskshub_solr_schema_definitions().
  $index_related_items_fields = idskshub_index_related_items_fields();
  $hub_fields_definitions = idskshub_get_solr_hub_fields_definitions();
  $required_fields = idskshub_get_solr_hub_required_fields();
  $hub_solr_schema = idskshub_get('hub_solr_schema', array('types'=>array(), 'single'=>array()));
  if ($language_codes = array_keys(idskshub_supported_languages())) {
    $language_codes_regexp = '/(' . implode('|', $language_codes) . ')/';
  }
  else {
    $language_codes_regexp = '/([a-z]){2}/';
  } 
  switch ($entity_type) {
    /*** Index Sources ***/
    case 'ids_ks_source':
      $prefix_sources_identifier = 'S';		// S for source!
      foreach ($documents as $document) {
        //watchdog('xx'); //print_r($document,true));
        // Initialize arrays with hub-generated fields.
        $fields_source = array();
        $fields_language = array();
        $hub_fields_values = array();
        $hub_values = array();
        $source_id_field = $document->getField('item_id');
        $source_id = $source_id_field['value'];
        $source = new IdsKsSource;
        $source->load($source_id); // Retrieve source object based on the entity id.
        if ($source->isSaved()) {
          // object_id
          $field_name = 'object_id' . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
          $field_value = $prefix_sources_identifier . $source_id;
          // Add the field to the index.
          $document->setField($field_name, $field_value); // For single-valued fields. Use $document->addField($field_name, $field_value) for multi-valued.
          // Add the field to the arrays that keep track of the fields values (indexed by source / language), in order to generate additional (search, count, filter, etc.) hub-generated fields, as defined in idskshub_get_solr_hub_fields_definitions() and idskshub_get_solr_hub_fields_values_definitions() functions, defined in idskshub.solrfields.inc.
          $fields_source['object_id'][IDSKS_OKHUB_SOURCE_CODE] = array($field_value);
          $fields_language['object_id'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($field_value); 
          // Call the function that generates the additional (hub) fields and adds them to the index.
          idskshub_solr_add_hub_fields('object_id', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);

          // item_id
          // $document->setField('item_id', $source_id); - It's not necessary to add the field, as it's already in the document.
          // Generated fields based on item_id - is this needed by the API?
          $fields_source['item_id'][IDSKS_HUB_SOURCE_CODE] = array($source_id);
          $fields_language['item_id'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($source_id); 
          idskshub_solr_add_hub_fields('item_id', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          
          // item_type / hub_item_type
          // Add field to the index.
          $document->setField('item_type', 'Source');
          // Generated fields based on item_type - is this needed by the API?
          $fields_source['item_type'][IDSKS_HUB_SOURCE_CODE] = array('Source');
          $fields_language['item_type'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array('Source'); 
          idskshub_solr_add_hub_fields('item_type', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          
          // Title
          $field_name = 'title' . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_DEFAULT_LANGUAGE;
          $document->setField($field_name, $source->get('name'));
          $fields_source['title'][IDSKS_OKHUB_SOURCE_CODE] = array($source->get('name'));
          $fields_language['title'][IDSKS_DEFAULT_LANGUAGE] = array($source->get('name')); 
          idskshub_solr_add_hub_fields('title', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          
          // object_type
          $field_name = 'object_type' . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
          $field_value = 'Source';
          $document->setField($field_name, $field_value);
          // This field is not used to generate additional fields. If needed, use the same code as object_id replacing the field name.

          // Content types available for this source
          if ($types_names = idskshub_get_types_source($source_id)) {
            $field_name = 'content_types' . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
            foreach ($types_names as $type_name) {
              $document->addField($field_name, $type_name);
            }
            $fields_source['content_types'][IDSKS_OKHUB_SOURCE_CODE] = $types_names;
            $fields_language['content_types'][IDSKS_NOT_APPLICABLE_LANGUAGE] = $types_names; 
            idskshub_solr_add_hub_fields('content_types', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          }

          // Examples: contributor's name and acronym used to generate additional hub fields (search, facet) - as defined in idskshub.solrfields.inc).
          if ($source->get('contributor')) {
            if ($source->get('contributor')->get('name')) {
              $fields_source['contributor'][IDSKS_OKHUB_SOURCE_CODE] = array($source->get('contributor')->get('name'));
              $fields_language['contributor'][IDSKS_DEFAULT_LANGUAGE] = array($source->get('contributor')->get('name')); 
              idskshub_solr_add_hub_fields('contributor', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            }
            if ($source->get('contributor')->get('acronym')) {
              $fields_source['acronym'][IDSKS_OKHUB_SOURCE_CODE] = array($source->get('contributor')->get('acronym'));
              $fields_language['acronym'][IDSKS_DEFAULT_LANGUAGE] = array($source->get('contributor')->get('acronym')); 
              idskshub_solr_add_hub_fields('acronym', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            }
            if ($source->get('contributor')->get('description')) {
              $fields_source['description'][IDSKS_OKHUB_SOURCE_CODE] = array($source->get('contributor')->get('description'));
              $fields_language['description'][IDSKS_DEFAULT_LANGUAGE] = array($source->get('contributor')->get('description')); 
              idskshub_solr_add_hub_fields('description', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            }	
           
          }

          // language
          if ($source->get('default_language') && ($language_values = idskshub_get_metadata_languages_fields($source->get('default_language')))) {
            $language_xml_field_value = '<language>' . $language_values . '</language>';
            $indexed_name = 'default_language' . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
            $indexed_value = "<languageList>$language_xml_field_value</languageList>";
            $document->setField($indexed_name, $indexed_value);        
          }

          // other generic fields (see particular cases in http://data.okhub.org/fields-indexing)
          foreach ($source as $field_name => $field_value) {
            if ($field_value && !(isset($fields_types[$field_name]) && ($fields_types[$field_name] == 'ignore'))) {
              // Need to make sure code is stored in the same format as the code when attached to items.
              if ($field_name == 'code') { 
                $field_value = preg_replace('/_/', '', $field_value);
              }
              elseif ($field_name == 'logo') {
                foreach ($field_value as $key_logo => $info_logo) {
                  if (is_string($info_logo) && preg_match('/^public:\/\//', $info_logo)) { 
                    $field_value[$key_logo] = preg_replace('/public:\/\/(.+)/', file_create_url('public://') . '$1', $info_logo);
                  }
                }
              }
              if (is_scalar($field_value)) {
                idskshub_add_solr_scalar($document, $field_name, $field_value, TRUE, $fields_types, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);    
              }
              elseif (is_array($field_value)) {
                idskshub_add_solr_array($document, $field_name, $field_value, $fields_types, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
              }
              elseif (is_object($field_value)) {
                if (isset($field_value->vocabulary_machine_name)) { // it's a vocabulary term.
                  if (isset($field_value->name)) {
                    idskshub_add_solr_scalar($document, $field_name, $field_value->name, TRUE, $fields_types, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);              
                  }
                }
                else {
                  if ($object_xml = idskshub_get_object_xml($field_name, $field_value)) {
                    $indexed_name = $field_name . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                    $indexed_value = '<' . $field_name . 'List>' . $object_xml . '</' . $field_name . 'List>';
                    $document->setField($indexed_name, $indexed_value);        
                  }
                }
              }
            }
          }
        }
      }
      break;
    /*** Index Items ***/
    case 'ids_ks_item':
      $path_title = 'title'; // Mapped path to look for titles in (used to include the titles of related items such as categories). TODO: Generalise!. See idskshub_index_related_items_fields().
      $metadata_xml_field_name = IDSKS_LANGUAGES_XML_FIELD . '_' . IDSKS_HUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
      $titles_items = array();
      //$count = 1;
      foreach ($documents as $document) {
        //$mem = number_format(memory_get_usage());
        //debug($mem, 'memory');
        $metadata_languages = array();
        $has_metadata_language_field = array();
        $fields_source = array();
        $fields_language = array();
        $hub_fields_values = array();
        $hub_values = array();
        $available_sources = array();
        $item_id = $document->getField('item_id');
        $item = new IdsKsItem;
        $item->load($item_id['value']);
        //if ($count == 1) { watchdog(WATCHDOG_NOTICE, print_r(get_class($item),true)); $count++; }
        if ($item->isSaved() && !$item->isEmptyItem()) {
          // Generate fields based on item_id. It is already indexed, so there's no need to call setField.
          $fields_source['item_id'][IDSKS_HUB_SOURCE_CODE] = array($item_id['value']);
          $fields_language['item_id'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($item_id['value']); 
          idskshub_solr_add_hub_fields('item_id', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);

          // Set item type
          if ($item->get('type') && $item->get('type')->get('name')) { 
            $item_type = $item->get('type')->get('name');
          }
          else {
            $item_type = IDSKS_DEFAULT_TYPE_ITEMS;
            drupal_set_message(t('Not valid type assigned to item with id @item_id. @default_type assumed', array('@item_id'=>$item_id,'@default_type'=>IDSKS_DEFAULT_TYPE_ITEMS)), 'warning');
          }
          $fields_source['item_type'][IDSKS_HUB_SOURCE_CODE] = array($item_type);
          $fields_language['item_type'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($item_type); 
          $document->setField('item_type', $item_type);
          idskshub_solr_add_hub_fields('item_type', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);

          // Set top_level and context - only for hierarchical items (themes and subjects) now. 
          if ($item_type == 'Theme' || $item_type == 'Subject') {
            $top_level = ($item->isTopLevel()) ? 'true' : 'false';
            $fields_source['top_level'][IDSKS_HUB_SOURCE_CODE] = array($top_level);
            $fields_language['top_level'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($top_level); 
            $top_level_field = 'top_level_' . IDSKS_HUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
            $document->setField($top_level_field, $top_level);
            idskshub_solr_add_hub_fields('top_level', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            // Set context values (by source) - Only for themes / subjects.
            $context_values = idskshub_get_context_fields($item); // Function defined in idskshub.solrfields.inc
            $context_field_name = 'hub_'. strtolower($item_type) .'_context';
            foreach ($context_values as $source => $context) {
              $fields_source[$context_field_name][$source] = array($context);
              $fields_language[$context_field_name][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($context);
              $context_field = $context_field_name . '_' . $source . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
              idskshub_add_solr_not_duplicated($document, $context_field, $context, $hub_solr_schema);
            }
            idskshub_solr_add_hub_fields($context_field_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          }
          // Set literals' values
          if ($item->get('versions_literals')) {
            $global_language_sources = array();
            foreach ($item->get('versions_literals') as $versions_literal) {
              if ($versions_literal && $versions_literal->get('path')) { // See: Change to index only mapped paths???
                $tag_name = array_pop(explode('/', $versions_literal->get('path')));
                $fields_source[$tag_name] = array();
                $fields_language[$tag_name] = array();
                if ($versions_literal->get('literals') && !(isset($fields_types[$tag_name]) && ($fields_types[$tag_name] == 'ignore'))) {
                  foreach ($versions_literal->get('literals') as $literal) {
                    if ($literal) {
                      if ($literal->get('source')) {
                        $source_id = $literal->get('source')->getId();
                        $source_name = $literal->get('source')->get('code');
                        // Excluding hub's values from the index (manually added items/values or created via mappings, for instance).
                        if ($source_name === IDSKS_HUB_SOURCE_CODE) {
                          $skip_literal = TRUE;
                        }
                        else {
                          $skip_literal = FALSE;
                          $source_name = preg_replace('/_/', '', $source_name); //we need to remove _ for the API.
                          if ($tag_name == IDSKS_LANGUAGES_FIELD) {
                            $has_metadata_language_field[] = $source_name;
                          }
                          if (!$literal->get('source')->get('is_semantic')) {
                            $available_sources[] = $source_name;
                          }
                          $field_name =  $tag_name . '_' . $source_name;      
                          $lang = FALSE;
                          if (in_array($tag_name, $fields_translatable)) {
                            if ($literal->get('attributes')) {
                              foreach ($literal->get('attributes') as $attribute) {
                                if ($attribute->get('name') == IDSKS_LANGUAGE_ATTRIBUTE) {
                                  if (preg_match($language_codes_regexp, $attribute->get('value'), $matches)) {
                                    $lang = $matches[0];
                                    break;
                                  }
                                }
                              }
                            }
                            // if there's no language set for the literal, we use the source's global language for the item.
                            if (!$lang) {
                              if (!isset($global_language_sources[$source_name])) {
                                $global_language_sources[$source_name] = $item->getGlobalLanguage($literal->get('source'));
                              }
                              $lang = $global_language_sources[$source_name];
                            }
                            if ($lang) {
                              if (!isset($metadata_languages[$source_name]) || !in_array($lang, $metadata_languages[$source_name])) {
                                $metadata_languages[$source_name][] = $lang;
                              }
                            }
                            else {
                              $lang = IDSKS_UNDEFINED_LANGUAGE_INDEX;
                            }
                          }
                          else {
                            $lang = IDSKS_NOT_APPLICABLE_LANGUAGE;
                          }
                        }
                      }
                      if (!$skip_literal && !is_null($literal->get('value'))) {
                        $field_name = $field_name . '_' . $lang;
                        if (isset($fields_types[$tag_name])) {
                          $value = idskshub_solr_field_assign_type($literal->get('value'), $fields_types[$tag_name]);
                        }
                        else {
                          $value = $literal->get('value');
                        }
                        if (isset($value)) {                   
                          idskshub_preprocess_solr_values($field_name, $value, $versions_literal->get('path'));
                          if ($value) {
                            $fields_source[$tag_name][$source_name][] = $value;
                            $fields_language[$tag_name][$lang][] = $value;
                            $hub_values[$tag_name][$source_name][$lang][] = $value;
                            idskshub_add_solr_not_duplicated($document, $field_name, $value, $hub_solr_schema);
                          }
                        }
                      }
                    } 
                  }
                }
                if (($tag_name != IDSKS_LANGUAGES_FIELD) && ($tag_name != IDSKS_SOURCES_FIELD)) { // These fields are processed below.
                  idskshub_solr_add_hub_fields($tag_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
                }
              }
            }
            // Add information about metadata languages present in this doc.
            foreach ($metadata_languages as $source_name_meta => $metadata_languages_source) {
              if (!in_array($source_name_meta, $has_metadata_language_field)) {
                $metadata_field_name = IDSKS_LANGUAGES_FIELD . '_' . $source_name_meta . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                foreach ($metadata_languages_source as $language) {
                  $fields_source[IDSKS_LANGUAGES_FIELD][$source_name_meta][] = $language;
                  $fields_language[IDSKS_LANGUAGES_FIELD][IDSKS_NOT_APPLICABLE_LANGUAGE][] = $language;
                  $hub_values[IDSKS_LANGUAGES_FIELD][$source_name_meta][IDSKS_NOT_APPLICABLE_LANGUAGE][] = $language;
                  idskshub_add_solr_not_duplicated($document, $metadata_field_name, $language, $hub_solr_schema);              
                }
              }
            }
            idskshub_solr_add_hub_fields(IDSKS_LANGUAGES_FIELD, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            // Add metatada_languages information as an XML field.
            $metadata_xml_field_value = '';
            $metadata_xml_languages = array_unique(idskshub_flatten_array($metadata_languages));
            foreach ($metadata_xml_languages as $language) {
              if ($metadata_language_fields = idskshub_get_metadata_languages_fields($language)) {
                $metadata_xml_field_value .= '<language>' . $metadata_language_fields . '</language>';
              }
            }
            if ($metadata_xml_field_value) {
              $metadata_xml_field_value = "<languageList>$metadata_xml_field_value</languageList>";
              idskshub_add_solr_not_duplicated($document, $metadata_xml_field_name, $metadata_xml_field_value, $hub_solr_schema);              
              idskshub_solr_add_hub_fields($metadata_xml_field_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            }
            // Add information about sources present in this doc.
            foreach (array_unique($available_sources) as $available_source) {
              idskshub_add_solr_not_duplicated($document, IDSKS_SOURCES_FIELD, $available_source, $hub_solr_schema);
            }
            idskshub_solr_add_hub_fields(IDSKS_SOURCES_FIELD, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          }
          // Add required fields
          foreach ($required_fields as $req_field => $existing_field) {
            if (!isset($hub_values[$req_field]) && isset($hub_values[$existing_field])) {
              $existing_values = $hub_values[$existing_field];
              foreach ($existing_values as $source_name_req => $lang_values) {
                foreach ($lang_values as $lang => $values) {
                  $field_name = $req_field . '_' . $source_name_req . '_' . $lang;
                  foreach ($values as $value) {
                    $fields_source[$req_field][$source_name_req][] = $value;
                    $fields_language[$req_field][$lang][] = $value;            
                    idskshub_add_solr_not_duplicated($document, $field_name, $value, $hub_solr_schema);
                  }
                } 
              }
              idskshub_solr_add_hub_fields($req_field, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            }
          }
          // Set date created/updated 
          if (is_array($item->get('track_changes')) && $changes = array_filter($item->get('track_changes'))) {  
            $latest_change = end($changes);
            if ($latest_change->get('timestamp')) {
              $timestamp = date("Y-m-d\Th:i:s\Z", $latest_change->get('timestamp'));
              $document->setField('hub_timestamp', $timestamp);
              idskshub_solr_add_hub_fields('hub_timestamp', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            }
          }   
          // Add related_items' content.
          $title_search_field_values = array();
          if ($item->get('related_items')) {
            $related_items = array();
            $related_items_xml = array();
            $combined_related_items_index_xml = array();
            // We iterate twice. The first time to get the sources for which the relationships are established and the second one to add the fields.
            foreach ($item->get('related_items') as $related_item) {
              if ($related_item && $related_item->get('referred_item') && $related_item->get('source')) {
                $referred_item_id = $related_item->get('referred_item')->getId();
                $related_items[$referred_item_id][] = $related_item->get('source')->get('code');
                if (!isset($titles_items[$referred_item_id])) {
                  $titles_items[$referred_item_id] = idskshub_get_item_display_fields_solr($related_item->get('referred_item'), $path_title);
                  if ($related_item->get('referred_item')->get('related_items')) {
                    foreach ($related_item->get('referred_item')->get('related_items') as $ref_rel_item) {
                      if ($ref_rel_item && $ref_rel_item->get('referred_item') && $ref_rel_item->get('relationship') && preg_match('/^same_as/', $ref_rel_item->get('relationship')->get('code'))) { // Adding related items linked with a same_as relationship.
                        $titles_items[$referred_item_id] += idskshub_get_item_display_fields_solr($ref_rel_item->get('referred_item'), $path_title);
                      }
                    }
                  }
                }
              }
            }
            foreach ($item->get('related_items') as $related_item) {
              if ($related_item && $related_item->get('referred_item') && $related_item->get('source')) {
                $referred_item_id = $related_item->get('referred_item')->getId();
                foreach ($titles_items[$referred_item_id] as $related_item_source_code => $title_field) {
                  /*------------ Add related items' titles - Only for documents and organizations -----------*/
                  if ($item_type == 'Document' || $item_type == 'Organisation') {
                    if (in_array($related_item_source_code, $related_items[$referred_item_id])) {
                      $prefix = IDSKS_HUB_SOURCE_CODE;
                    }
                    else {
                      $prefix = 'mapped';
                    }
                    $related_item_source_code = preg_replace('/_/', '', $related_item_source_code);
                    // Add related item's title to source-specific fields.
                    $title_field_name = $prefix . '_' . $title_field['type'] . '_' . $related_item_source_code . '_' . $title_field['language'];
                    idskshub_add_solr_not_duplicated($document, $title_field_name, $title_field['value'], $hub_solr_schema);
                    // Add item's title to hub-generated combined fields (search and facet).
                    /* Uncomment to use these values to generate hub fields defined in idskshub.solrfields.inc (not being used now, as the search and facet fields are generated here, below).
                    $fields_source[$title_field['type']][$related_item_source_code][] = $title_field['value'];
                    $fields_language[$title_field['type']][$title_field['language']][] = $title_field['value'];
                    idskshub_solr_add_hub_fields($title_field['type'], $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
                    */
                    /* Add title search and facet fields.
                      Alternatively, these could be defined in idskshub.solrfields.inc. In this case, the lines above should be uncommented.
                      But we would need to add field definitions for each item type / language combination.
                      And this would need to be done each time a new item type is added.
                      For instance (in idskshub_get_solr_hub_fields_definitions()):
                        ...
                        'region' => array(
                            'hub_region_search_hub_en' => array('construction' => 'combine'), // And add filter in idskshub_get_solr_hub_fields_values_definitions(). The same for all the language codes.
                            'hub_region_search_hub_zx' => array('construction' => 'combine'),
                            'hub_region_facet_hub_zx' => array('copy' => 'region_search_hub_zx'),
                        ),
                        ...
                    */
                    // Add title to combined search field by language.
                    $title_search_field_name = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_search_hub_' . $title_field['language'];
                    if (!isset($title_search_field_values[$title_search_field_name]) || !in_array($title_field['value'], $title_search_field_values[$title_search_field_name])) {
                      $title_search_field_values[$title_search_field_name][] = $title_field['value'];
                      idskshub_add_solr_not_duplicated($document, $title_search_field_name, $title_field['value'], $hub_solr_schema);
                      $hub_fields_values[$title_field['type']][] = $title_field['value']; // Keep track of these other hub-generated fields.
                    }
                    // Add title to combined search field by all languages.
                    $title_search_field_name_multilingual = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_search_hub_' . IDSKS_UNDEFINED_LANGUAGE_INDEX;
                    if (!isset($title_search_field_values[$title_search_field_name_multilingual]) || !in_array($title_field['value'], $title_search_field_values[$title_search_field_name_multilingual])) {
                      $title_search_field_values[$title_search_field_name_multilingual][] = $title_field['value'];
                      idskshub_add_solr_not_duplicated($document, $title_search_field_name_multilingual, $title_field['value'], $hub_solr_schema);
                    }
                    // Add title to combined facet field.
                    $title_facet_field_name = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_facet_hub_' . IDSKS_UNDEFINED_LANGUAGE_INDEX;
                    if (!isset($title_search_field_values[$title_facet_field_name]) || !in_array($title_field['value'], $title_search_field_values[$title_facet_field_name])) {
                      $title_search_field_values[$title_facet_field_name][] = $title_field['value'];
                      idskshub_add_solr_not_duplicated($document, $title_facet_field_name, $title_field['value'], $hub_solr_schema);
                    }
                  }
                  /*------------ End related items' titles -----------*/

                  /*-=----------- Add related items' ids - Only for documents and organizations -------------*/
                  if ($item_type == 'Document' || $item_type == 'Organisation') {
                    /* Uncomment in order to generate source-specific fields with the ids, such as hub_theme_item_id_eldis_zz.
                    // Add related item's id to source specific-fields.
                    $id_field_name = $prefix . '_' . $title_field['type'] . '_item_id_' . $related_item_source_code . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                    // Add the Hub one here? $id_field_name = $prefix . '_' . $title_field['type'] . '_item_id_' . IDSKS_HUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                    idskshub_add_solr_not_duplicated($document, $id_field_name, $referred_item_id, $hub_solr_schema);
                    */
                    // Add related item's id to combined (search and facet) fields.
                    /* Uncomment to use these values to generate hub fields defined in idskshub.solrfields.inc (not being used now, as the search and facet fields are generated here, below).
                    $fields_source[$title_field['type'] . '_item_id'][$related_item_source_code][] = $referred_item_id;
                    $fields_language[$title_field['type'] . '_item_id'][IDSKS_NOT_APPLICABLE_LANGUAGE][] = $referred_item_id;
                    idskshub_solr_add_hub_fields($title_field['type'] . '_item_id', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
                    */
                    /* Generating search and facet fields with the related item's ids */
                    // Ids search field.
                    $id_search_field_name = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_item_id_search_hub_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                    idskshub_add_solr_not_duplicated($document, $id_search_field_name, $referred_item_id, $hub_solr_schema);
                    // Ids facet field.
                    $id_facet_field_name = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_item_id_facet_hub_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                    idskshub_add_solr_not_duplicated($document, $id_facet_field_name, $referred_item_id, $hub_solr_schema);
  
                    // Add related item's id to combined field (for instance: 'hub_theme_item_id_hub_zz')
                    $id_hub_field_name = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_item_id_hub_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                    idskshub_add_solr_not_duplicated($document, $id_hub_field_name, $referred_item_id, $hub_solr_schema);
                    $hub_fields_values[IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] . '_item_id'][] = $referred_item_id; // Keep track of these other hub-generated fields.
                  }
                  /*------------- End related items' ids -------------*/

                  /*------ Generate related_items' fields values as XML  ------*/
                  $ref_item_rel_items = $related_item->get('referred_item')->getRelItemsIds();
                  if ($related_item->get('source')->get('code') == IDSKS_OKHUB_SOURCE_CODE) {
                    $related_item_source_code = IDSKS_OKHUB_SOURCE_CODE;
                  }
                  if (in_array($related_item_source_code, $available_sources) || $related_item_source_code == IDSKS_OKHUB_SOURCE_CODE) { // Now we are only adding the related item's fields for the sources present in the item, with the exception of relationships established by the hub.
                    foreach (array('all_sources', $related_item_source_code) as $index_related_item_source_code) {
                      if (isset($index_related_items_fields[$index_related_item_source_code][$related_item->get('relationship')->get('code')])) {
                        $related_item_xml = '';
                        $related_items_xml_definition = $index_related_items_fields[$index_related_item_source_code][$related_item->get('relationship')->get('code')];
                        $related_item_values = $related_item->get('referred_item')->getValuesBySource();
                        $indexed_field_name = $related_items_xml_definition['indexed_field_name'];
                        $related_item_fields = $related_items_xml_definition['related_item_fields'];
                        if ($indexed_field_name && $related_item_fields) {
                          if (isset($related_items_xml_definition['related_item_wrappers'])) {
                            $related_item_wrappers = $related_items_xml_definition['related_item_wrappers'];
                          }
                          elseif ($related_item->get('referred_item')->get('type') && $related_item->get('referred_item')->get('type')->get('name')) {
                            $related_item_wrappers = strtolower($related_item->get('referred_item')->get('type')->get('name'));
                          }
                          else {
                            $related_item_wrappers = 'list-item';
                          }
                          if (isset($related_items_xml_definition['external_wrappers'])) {
                            $external_wrappers = $related_items_xml_definition['external_wrappers'];
                          }
                          elseif ($related_item->get('referred_item')->get('type') && $related_item->get('referred_item')->get('type')->get('name')) {
                            $external_wrappers = strtolower($related_item->get('referred_item')->get('type')->get('name')) . 'List';
                          }
                          else {
                            $external_wrappers = 'list';
                          }
                          $related_items_xml_external_wrappers[$index_related_item_source_code][$indexed_field_name] = $external_wrappers;
                          foreach ($related_item_fields as $related_item_field_string_paths => $related_item_field_name) {
                            $related_item_field_paths = explode('|', $related_item_field_string_paths);
                            foreach ($related_item_field_paths as $related_item_field_path) {
                              if (preg_match('/%(\w+)%/', $related_item_field_path, $matches)) {
                                $related_item_field_value = idskshub_get_related_item_meta_field($related_item, $matches[1], $ref_item_rel_items);
                                if (isset($related_item_field_value)) {
                                  $related_item_xml .= "<$related_item_field_name>$related_item_field_value</$related_item_field_name>";
                                  break;
                                }
                              }
                              elseif (!empty($related_item_values[$related_item_source_code][$related_item_field_path])) {
                                $related_item_field_value = end($related_item_values[$related_item_source_code][$related_item_field_path]); // Get the latest. TODO: See languages.
                                $related_item_xml .= "<$related_item_field_name>" . htmlspecialchars($related_item_field_value) . "</$related_item_field_name>";
                                break;
                              }
                            }                            
                          }
                          if ($related_item_xml) {
                            if (is_array($related_item_wrappers)) {
                              foreach ($related_item_wrappers as $related_item_wrapper) {
                                $related_item_xml = "<$related_item_wrapper>$related_item_xml</$related_item_wrapper>";
                              }
                            }
                            else {
                              $related_item_xml = "<$related_item_wrappers>$related_item_xml</$related_item_wrappers>";
                            }
                            if (!isset($related_items_xml[$related_item_source_code][$indexed_field_name]) || !in_array($related_item_xml, $related_items_xml[$related_item_source_code][$indexed_field_name])) {
                              $related_items_xml[$related_item_source_code][$indexed_field_name][] = $related_item_xml;
                            }
                          }
                        }
                      }
                    }
                  }
                  /*------ End related_items' fields values as XML ------*/
                }
              }
            } // end related_items loop.
            // Add the related items' generated XML values to the fields to index.
            foreach ($related_items_xml as $source_code_referred_item => $related_items_field_xml) {
              foreach ($related_items_field_xml as $indexed_field_name => $list_related_items_xml) {
                if ($related_items_index_xml = implode($list_related_items_xml)) {
                  if (!isset($combined_related_items_index_xml[$indexed_field_name])) {
                    $combined_related_items_index_xml[$indexed_field_name] = '';
                  }
                  $combined_related_items_index_xml[$indexed_field_name] .= $related_items_index_xml;
                  if (isset($related_items_xml_external_wrappers[$source_code_referred_item][$indexed_field_name])) {
                    $external_wrappers = $related_items_xml_external_wrappers[$source_code_referred_item][$indexed_field_name];
                  }
                  else {
                    $external_wrappers = $related_items_xml_external_wrappers['all_sources'][$indexed_field_name];
                  }
                  if (is_array($external_wrappers)) {
                    foreach ($external_wrappers as $external_wrapper) {
                      $related_items_index_xml = "<$external_wrapper>$related_items_index_xml</$external_wrapper>";
                    }
                  }
                  else {
                    $related_items_index_xml = "<$external_wrappers>$related_items_index_xml</$external_wrappers>";
                  }
                  $source_name_referred_item = preg_replace('/_/', '', $source_code_referred_item);
                  $indexed_field_name_source_language = $indexed_field_name . '_' . $source_name_referred_item . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                  idskshub_preprocess_solr_values($indexed_field_name_source_language, $related_items_index_xml);
                  $document->setField($indexed_field_name_source_language, $related_items_index_xml);
                  /* Uncomment in order to use these fields to generate hub fields in idskshub.solrfields.inc
                  $fields_source[$indexed_field_name][$source_code_referred_item] = array($related_items_index_xml);
                  $fields_language[$indexed_field_name][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($related_items_index_xml); 
                  idskshub_solr_add_hub_fields($indexed_field_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
                  */
                }
              }
            }
            // Add combined XML for the related items.
            foreach ($combined_related_items_index_xml as $indexed_field_name => $combined_items_xml) {
              if (isset($related_items_xml_external_wrappers['all_sources'][$indexed_field_name])) {
                $external_wrappers = $related_items_xml_external_wrappers['all_sources'][$indexed_field_name];
                if (is_array($external_wrappers)) {
                  foreach ($external_wrappers as $external_wrapper) {
                    $combined_items_xml = "<$external_wrapper>$combined_items_xml</$external_wrapper>";
                  }
                }
                else {
                  $combined_items_xml = "<$external_wrappers>$combined_items_xml</$external_wrappers>";
                }
                $indexed_field_name_source_language = $indexed_field_name . '_' . IDSKS_HUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                idskshub_preprocess_solr_values($indexed_field_name_source_language, $combined_items_xml);
                $hub_fields_values[$indexed_field_name][] = $combined_items_xml; // Keep track of these other hub-generated fields, just in case.
                // Uncoment to use this value to generate other fields.
                // idskshub_solr_add_hub_fields($indexed_field_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
                $document->setField($indexed_field_name_source_language, $combined_items_xml);
              }
            }
          }
        }
      }
      break; // End case entity type is 'ids_ks_item'.
  } // switch
  //debug($documents, 'Indexing...', TRUE);
}

// Used to retrieve "titles" from item's literals - including items related to it with "same as". Used to add related items' titles in search and facet fields.
function idskshub_get_item_display_fields_solr($item, $field) {
  $display_fields = array();
  if ($type = $item->get('type')) {
    $field_name = strtolower(preg_replace('/([^A-Z])([A-Z])/', '\1_\2', $type->get('name')));
    $display_paths =  $item->getDisplayFieldValues($field);
    foreach ($display_paths as $path => $language_values) {
      foreach ($language_values as $language => $sources_values) {
        foreach ($sources_values as $source_code => $source_values) {
          if (!empty($source_values)) {
            $display_fields[$source_code] = array('type' => $field_name, 'language' => $language, 'value' => $source_values[0]);
          }
        }
      }
    } 
  }
  return $display_fields;
}

// Implements hook_search_api_solr_query_alter()
function idskshub_search_api_solr_query_alter(array &$call_args, SearchApiQueryInterface $query) {
  $index = $query->getIndex();
  if ($index->item_type == 'ids_ks_item') {
    $call_args['params']['fl'] = array('*', 'score');
    //$call_args['params']['hl'] = array('true');
    //$call_args['params']['hl.fl'] = array('*');
  }
}

// Implements hook_search_api_solr_search_results_alter()
function idskshub_search_api_solr_search_results_alter(&$results, $query, $response) {
  $index = $query->getIndex();
  if ($index->item_type == 'ids_ks_item') {
    foreach ($response->response->docs as $document) {
      foreach ($document as $field => $value) {
        $results['results'][$document->item_id]['fields'][$field] = $value;
      }
    }
  }
}

/* ------------ Additional functions ------------- */

function idskshub_add_solr_scalar($document, $field_name, $field_value, $single, $fields_types, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema) {
  if (isset($fields_types[$field_name])) { // For fields defined as int, date, etc. - to make sure that they have the right format.
    $field_value = idskshub_solr_field_assign_type($field_value, $fields_types[$field_name]);
  }
  if (in_array($field_name, $fields_translatable)) {
    $language = IDSKS_DEFAULT_LANGUAGE;
    $indexed_name = $field_name . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_DEFAULT_LANGUAGE; // See in case these values are added in multiple/other language at some point.
  }
  else {
    $language = IDSKS_DEFAULT_LANGUAGE;
    $indexed_name = $field_name . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
  }
  if ($single) {
    $document->setField($indexed_name, $field_value);
  }
  else {
    $document->addField($indexed_name, $field_value);
  }
  // Use it to (potentially), generate additional hub field.
  $fields_source[$field_name][IDSKS_OKHUB_SOURCE_CODE] = array($field_value);
  $fields_language[$field_name][$language] = array($field_value); 
  idskshub_solr_add_hub_fields($field_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
}

function idskshub_add_solr_array($document, $field_name, $arr, $fields_types, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema) { // The only possibility is that it's a non-associative array of scalars or objects.
  $xml = '';
  foreach ($arr as $field_value) {
    if (is_scalar($field_value)) {
      idskshub_add_solr_scalar($document, $field_name, $field_value, FALSE, $fields_types, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);              
    }
    elseif (is_object($field_value)) { 
      if (isset($field_value->vocabulary_machine_name)) { // it's a vocabulary term.
        if (isset($field_value->name)) {
          idskshub_add_solr_scalar($document, $field_name, $field_value->name, FALSE, $fields_types, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);              
        }
      }
      else {
        $xml .= idskshub_get_object_xml($field_name, $field_value);
      }
    }
  }
  if ($xml) {
    $indexed_name = $field_name . '_' . IDSKS_OKHUB_SOURCE_CODE . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
    $indexed_value = '<' . $field_name . 'List>' . $xml . '</' . $field_name . 'List>';
    $document->addField($indexed_name, $indexed_value);
  }
}

function idskshub_get_object_xml($field_name, $obj) {
  $xml = '';
  foreach ($obj as $field => $value) { // In the future, idskshub_index_related_items_fields() could be use to indicate which properties to index.
    if (isset($value)) {
      if (is_scalar($value)) {
        $xml .= "<$field>" .  htmlspecialchars($value) . "</$field>";
      }
      elseif (is_array($value)) {
        if ($array_xml = idskshub_get_array_xml($field, $value)) {
          $xml .= '<' . $field . 'List>' . $array_xml . '</' . $field . 'List>';        
        }
      }
      elseif (is_object($value)) {
        if ($object_xml = idskshub_get_object_xml($field, $value)) {
          $xml .= '<' . $field . 'List>' . $object_xml . '</' . $field . 'List>';        
        }
      }
    }
  }
  if ($xml) {
    $xml = "<$field_name>" . $xml . "</$field_name>";
  }
  return $xml;
}

function idskshub_get_array_xml($field_name, $arr) {
  $is_assoc = count(array_filter(array_keys($arr), 'is_string'));
  $xml = '';
  foreach ($arr as $key => $value) {
    $field = ($is_assoc) ? $key : 'list-item';
    if (is_scalar($value)) {
      $xml .= "<$field>" .  htmlspecialchars($value) . "</$field>";
    }
    elseif (is_array($value)) {
      if ($array_xml = idskshub_get_array_xml($field, $value)) {
        $xml .= '<' . $field . 'List>' . $array_xml . '</' . $field . 'List>';        
      }
    }
    elseif (is_object($value)) {
      if ($object_xml = idskshub_get_object_xml($field, $value)) {
        $xml .= '<' . $field . 'List>' . $object_xml . '</' . $field . 'List>';        
      }
    }
  }
  if ($xml) {
    $xml = "<$field_name>" . $xml . "</$field_name>";
  }
  return $xml;
}

// Add item's value to the index, making sure that no duplicated values are generated.
function idskshub_add_solr_not_duplicated($document, $field_name, $field_value, $hub_solr_schema) {
  $item_id = $document->getField('item_id');
  if ($existing_values = $document->getField($field_name)) {
    if (!idskshub_single_valued($field_name, $hub_solr_schema)) {
      if (!in_array($field_value, $existing_values['value'])) {
        $document->addField($field_name, $field_value);
      }
    }
    else {
      trigger_error(t('Multiple values found when indexing single-valued field "@field_name" in item @item_id. Field value ignored: "@field_value".', array('@field_name' => $field_name, '@item_id' => $item_id['value'], '@field_value' => $field_value)), E_USER_NOTICE);
    }
  }
  else {
    $document->addField($field_name, $field_value);
  }
}

// Indicates if a field is defined as single-valued in Solr's schema.
function idskshub_single_valued($field_name, $hub_solr_schema) {
  if (!$single = in_array($field_name, $hub_solr_schema['single'])) {
//    $dynamic_field_name = preg_replace('/(.+)_[^_]+_[^_]+/', '\1_*', $field_name);
//    $single = in_array($dynamic_field_name, $hub_solr_schema['single']);
    foreach ($hub_solr_schema['single'] as $single_field) {
      if (preg_match('/\*/', $single_field)) {
        $single_pattern = preg_replace('/\*/', '.*', $single_field);
        if ($single = preg_match("/^$single_pattern/", $field_name)) {
          break;
        }
      }
    }
  }
  return $single;
}

// Get the values of the fields addded by the hub (generic value(s), sort value and search/filter value(s)).
// If there are filters (language/source), we apply them. Otherwise, we consider all the values (for the default language, if set).
function idskshub_get_solr_hub_fields_values($field_name, $new_field_name, $construction, $fields_language, $fields_source, $fields_translatable) {
  $new_field_value = FALSE;
  $new_field_values = array();
  $new_field_values_source = array();
  $new_field_values_language = array();
  if (isset($fields_language[$field_name]) && isset($fields_source[$field_name])) {
    $hub_fields_values_definitions = idskshub_get_solr_hub_fields_values_definitions();
    $filters_def = $hub_fields_values_definitions[$construction];
    if (isset($filters_def[$new_field_name])) {
      $filters = $filters_def[$new_field_name];
      if (isset($filters['source'])) {
        if (isset($fields_source[$field_name][$filters['source']])) {
          $new_field_values_source = $fields_source[$field_name][$filters['source']];
        }
      }
      if (empty($new_field_values_source)) {
        $new_field_values_source = array_reduce($fields_source[$field_name], 'array_merge', array());
      }
      if (in_array($field_name, $fields_translatable)) {
        if (isset($filters['language'])) {
          $filter_language = $filters['language'];
        }
        else {
          $filter_language = IDSKS_DEFAULT_LANGUAGE; // TODO: if this is not set, consider all the values for all languages?
        }
        if ($filter_language == 'all') {
          $new_field_values_language = array_reduce($fields_language[$field_name], 'array_merge', array());
        }
        elseif (isset($fields_language[$field_name][$filter_language])) {
          $new_field_values_language = $fields_language[$field_name][$filter_language];
        }
        if (empty($new_field_values_language)) {
          $new_field_values_language = array_reduce($fields_language[$field_name], 'array_merge', array());
        }
        $new_field_values = array_intersect($new_field_values_source, $new_field_values_language);
        if (empty($new_field_values)) {
          if (isset($filters['source']) && isset($filters['priority']) && $filters['priority'] == 'source') {
            $new_field_values = $new_field_values_source;
          }
          else {
            $new_field_values = $new_field_values_language;
          }
        }
      }
      else { // not translatable
        $new_field_values = $new_field_values_source; // Order ok?
      }
    }
    if (empty($new_field_values)) {
      $new_field_values = array_reduce($fields_language[$field_name], 'array_merge', array());
    }
    if (isset($filters['regexp'])) {
      $filtered_values = array();
      foreach ($new_field_values as $value) {
        if (preg_match($filters['regexp'], $value)) {
          $filtered_values[] = $value;
        }
      }
      $new_field_values = $filtered_values;
    }
    if (!empty($new_field_values)) {
      if ($construction == 'single') {
        if (isset($filters['select'])) {
          switch ($filters['select']) {
            case 'highest':
              $new_field_value = max($new_field_values);
              break;
            case 'lowest':
              $new_field_value = min($new_field_values);
              break;
            case 'first':
              $all_values_indexed = array_values($new_field_values); // just to make sure.
              $new_field_value = $all_values_indexed[0];
              break;
          }
        }
        if (empty($new_field_value) && $new_field_value !== "0") { // 'latest' is the default.
          $new_field_value = end($new_field_values);
        }
      }
      else { // not defined or 'combine'.
        $new_field_value = array_unique($new_field_values);
      }
    }
  }
  return $new_field_value;
}

// Add combined/sortable/searchable fields that have to be generated from $field_name's value, as defined in idskshub.solrfields.inc.
// Note: Because of the way in which items are updated, the values are ordered from oldest to most recent in $values.
function idskshub_solr_add_hub_fields($field_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, &$hub_fields_values, $hub_solr_schema) {
  $types_def = array_keys($hub_fields_definitions);
  foreach ($hub_fields_definitions as $hub_field_type => $fields_defs) { // $hub_fields_definitions['generic']['title_hub_zx']['construction']
    if (isset($fields_defs[$field_name])) {
      $field_defs = $fields_defs[$field_name];
      foreach ($field_defs as $new_field_name => $field_name_def) {
        $construction = '';
        $new_values = FALSE;
        if (isset($field_name_def['copy'])) {
          $field_name_to_copy = $field_name_def['copy'];
          if (isset($hub_fields_values[$field_name_to_copy])) {
            $new_values = $hub_fields_values[$field_name_to_copy];
          }
          foreach ($types_def as $type_def) {
            if (isset($hub_fields_definitions[$type_def][$field_name_to_copy]['construction'])) {
              $construction = $hub_fields_definitions[$type_def][$field_name_to_copy]['construction'];
              break;
            }
          }
        } elseif (isset($field_name_def['construction'])) {
          $construction = $field_name_def['construction'];
        }
        if (!$construction) {
          if ($hub_field_type == 'search') { // in this case the default is 'combine'. in all other cases ('generic', 'sort') it's 'single'.
            $construction = 'combine';
          }
          else {
            $construction = 'single';
          }
        }
        if (!isset($hub_fields_values[$new_field_name]) || ($construction != 'single')) {
          if (empty($new_values) && $new_values !== "0") {
            $new_values = idskshub_get_solr_hub_fields_values($field_name, $new_field_name, $construction, $fields_language, $fields_source, $fields_translatable);
          }
          if (!empty($new_values) || $new_values === "0") {
            $hub_fields_values[$new_field_name] = $new_values;
            if (is_array($new_values) && ($construction != 'single')) {
              foreach ($new_values as $new_value) {
                idskshub_add_solr_not_duplicated($document, $new_field_name, $new_value, $hub_solr_schema);
              }
            }
            elseif (is_scalar($new_values)) {
              idskshub_add_solr_not_duplicated($document, $new_field_name, $new_values, $hub_solr_schema);
            }
          }
        }
      }
    }
  }
}

// Checks format of values according to the fields types.
function idskshub_solr_field_assign_type($value, $field_type) {
  if (isset($value)) {
    switch ($field_type) {
      case 'int':
        $value = (int) $value;
        break;
      case 'date':
        $time = strtotime($value);
        if (empty($time)) { // try again, stripping out some things.
          $value = preg_replace('/.*?(\d+[\/|\-]\d+[\/|\-]\d+).*/','\1', $value);
          $time = strtotime($value);
        }
        if (empty($time)) { // still not valid, assign unix's epoch time to avoid errors and enable comparison.
          $time = strtotime('1');
        }
        $value =  date("Y-m-d\Th:i:s\Z", $time);
        break;
    }
  }
  return $value;
}
 
// Retrieve single-valued fields and field types from Solr's schema.
function idskshub_solr_schema_definitions() {
  $field_definitions = array();
  $solr_schema_url = IDSKS_SOLR_SERVER.'/solr/'.IDSKS_SOLR_CORE.'/admin/luke?show=schema';
  $curl_handle=curl_init();
  curl_setopt($curl_handle, CURLOPT_URL,$solr_schema_url);
  curl_setopt($curl_handle, CURLOPT_CONNECTTIMEOUT, 2);
  curl_setopt($curl_handle, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($curl_handle, CURLOPT_USERAGENT, 'okhub.org');
  $xml = curl_exec($curl_handle);
  if (curl_exec($curl_handle) === false) {
    trigger_error(t('Solr schema definition could not be retrieved from "@solr_schema_url": '. curl_error($curl_handle), array('@solr_schema_url' => $solr_schema_url)));
    curl_close($curl_handle);
  } else {
    
    if ($xml_schema = simplexml_load_string($xml)) {
      $json_schema = json_encode($xml_schema);
      $array_schema = json_decode($json_schema ,TRUE);
      $schema = $array_schema['lst'][2]['lst']; // $array_schema[2]['@attributes']['name'] == 'schema'
      $fields = $schema[0]['lst'];  // $schema[0]['@attributes']['name'] == 'fields';
      $dynamic_fields = $schema[1]['lst'];  // $schema[1]['@attributes']['name'] == 'fields';
      $field_definitions = array();
      foreach ($fields as $field) {
        $field_name = $field['@attributes']['name'];
        $field_type = $field['str'][0];
        $field_flags = $field['str'][1];   
        $field_definitions['types'][$field_name]  = $field_type;
        if (!preg_match('/...M........../i', $field_flags)) {
          $field_definitions['single'][] = $field_name;
        }
      }
      foreach ($dynamic_fields as $field) {
       $field_name = $field['@attributes']['name'];
       $field_type = $field['str'][0];
       $field_flags = $field['str'][1];
       $field_definitions['types'][$field_name]  = $field_type;
       if (!preg_match('/...M........../i', $field_flags)) {
        $field_definitions['single'][] = $field_name;
        }
      }
    }
  }
  return $field_definitions;
}

// Functions used to generated / modify values solely for indexing puroposes.

function idskshub_get_related_item_meta_field($rel_item, $meta_field, $ref_item_rel_items) {
  $value = NULL;
  switch ($meta_field) {
    case 'item_id':
      $value = $rel_item->get('referred_item')->getId();
      break;
    case 'item_type':
      if ($rel_item->get('referred_item')->get('type')) {
        $value = $rel_item->get('referred_item')->get('type')->get('name');
      }
      break;
    case 'language':
      if ($language = $rel_item->get('referred_item')->getGlobalLanguage($rel_item->get('source'))) {
        $value = $language;
      }
      break;
    case 'source':
      if ($source = $rel_item->get('referred_item')->getLatestSourceCode()) {
        $value = preg_replace('/_/', '', $source);
      }
      break;
    case 'top_level':
      if ($rel_item->get('source')->get('code') == IDSKS_OKHUB_SOURCE_CODE) {
        $value = ($rel_item->get('referred_item')->isTopLevel()) ? 'true' : 'false'; // Have no parents for any source.
      }
      else {
        $value = ($rel_item->get('referred_item')->isTopLevel($rel_item->get('source')->get('code'))) ? 'true' : 'false'; // Is top-level for this source.
      }
      break;
    case 'display_title':
      if ($rel_item->get('referred_item')->get('display_title')) {
        $value = htmlspecialchars($rel_item->get('referred_item')->get('display_title'));
      }
      break;
    case 'short_display_title':
      if ($rel_item->get('referred_item')->get('display_title')) {
        $value = htmlspecialchars(end(explode('.', $rel_item->get('referred_item')->get('display_title'))));
      }
      break;
    default:
      // See if it's an existing relationship code.
      if (IdsKsRelationship::getIds(array('code' => $meta_field))) {
        $ref_item_rel_items_ids = array();
        if (isset($ref_item_rel_items[$meta_field]) && $rel_item->get('source')->get('code') == IDSKS_OKHUB_SOURCE_CODE) { // If the source of the relationship is the hub, we include the related items for all sources.
          foreach ($ref_item_rel_items[$meta_field] as $source_code => $values_source) {
            $ref_item_rel_items_ids = array_merge($ref_item_rel_items_ids, $values_source);
          }
        }
        else {
          if (isset($ref_item_rel_items[$meta_field][$rel_item->get('source')->get('code')])) {
            $ref_item_rel_items_ids = $ref_item_rel_items[$meta_field][$rel_item->get('source')->get('code')];
          }
          if (isset($ref_item_rel_items[$meta_field][IDSKS_OKHUB_SOURCE_CODE])) {
            $ref_item_rel_items_ids = array_merge($ref_item_rel_items_ids, $ref_item_rel_items[$meta_field][IDSKS_OKHUB_SOURCE_CODE]);
          }
        }
        $ref_item_rel_items_ids = array_unique($ref_item_rel_items_ids);
        foreach ($ref_item_rel_items_ids as $ref_item_rel_item_id) {
          $value .= "<item_id>$ref_item_rel_item_id</item_id>";
        }
      }
  }
  return $value;
}

// TODO: Get this from the database and add admin interface.
function idskshub_preprocess_solr_values(&$field_name, &$field_value, $original_path='') {
  if (preg_match('/(.*)_(.*)_(.*)/', $field_name, $matches)) {
    $field = $matches[1];
    $source = $matches[2];
    $language = $matches[3];
    switch ($field) {    
      case 'description':
        $field_value = preg_replace('/<img.*beacon_guid.*>/', '', $field_value);
        break;
      case 'language_array': // <language_array><language><list-item> ==> <languageList><language>
        if (!preg_match('/languageList/', $field_value)) {
          $field_value = preg_replace('/<language>/', '', $field_value);
          $field_value = preg_replace('/<\/language>/', '', $field_value);
          $field_value = preg_replace('/<list\-item>/', '<language>', $field_value);
          $field_value = preg_replace('/<\/list\-item>/', '</language>', $field_value);
          $field_value = "<languageList>$field_value</languageList>";
        }
        if (!(preg_match('/\[CDATA\[/', $field_value))) {
          $field_value = preg_replace('/<description>/i', '<description><![CDATA[', $field_value);
          $field_value = preg_replace('/<\/description>/i', ']]></description>', $field_value);
        }
        break;
    }
    $field_value = preg_replace('/\>\s*/', '>', $field_value);
    $field_value = preg_replace('/\s*\</', '<', $field_value);
  }
}

function idskshub_get_solr_translatable() {
  $fields_translatable = idskshub_get('translatable_fields');
  return $fields_translatable;
}

// Fields with particular types that need to be considered to make sure that they are well-formatted when sent to Solr.
function idskshub_solr_fields_types() {
  $fields_types = idskshub_get('field_types');
  return $fields_types;
}

// Single-valued fields have to be included in the schema.
function idskshub_solr_fields_single_valued() {
  $fields_types = idskshub_get('field_types');
  return $fields_types;
}

