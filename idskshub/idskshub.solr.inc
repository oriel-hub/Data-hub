<?php
// $Id$

/**
 * @file
 *   IdsKsHub Solr functions.
 */

require_once('idskshub.solrfields.inc');

class IdsKsFilterNonTypedItems extends SearchApiAbstractAlterCallback {
  public function alterItems(array &$items) {
    $generic_type = new IdsKsType();
    $generic_type->name = IDSKS_GENERIC_TYPE_NAME;
    $generic_type->load();
    if ($generic_type->getId()) {
      foreach ($items as $id => &$item) {
        $entity_lang = $item->search_api_language;
        if (isset($item->field_item_type[$entity_lang][0]['target_id'])) {
          $type_id = $item->field_item_type[$entity_lang][0]['target_id'];
          if ($type_id == $generic_type->getId()) {
            unset($items[$id]);
          }
        }
      }
    }
  }
} // class IdsKsFilterNonTypedItems

class IdsKsFilterDocuments extends SearchApiAbstractAlterCallback {
  public function alterItems(array &$items) {
    $document_type = new IdsKsType();
    $document_type->name = 'Document';
    $document_type->load();
    if ($document_type->getId()) {
      foreach ($items as $id => &$item) {
        $entity_lang = $item->search_api_language;
        $type_id = $item->field_item_type[$entity_lang][0]['target_id'];
        if ($type_id != $document_type->getId()) {
          unset($items[$id]);
        }
      }
    }
  }
} // class IdsKsFilterDocuments

/* --------------- Hooks ---------------- */

// Implements hook_search_api_alter_callback_info().
function idskshub_search_api_alter_callback_info() {
  $callbacks['ids_ks_documents'] = array(
    'name' => t('IDS KS Documents'),
    'description' => t('Indexes only documents.'),
    'class' => 'IdsKsFilterDocuments',
  );
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
  $commit_url = 'http://'.IDSKS_SOLR_SERVER.'/solr/'.IDSKS_SOLR_CORE.'/update?commit=true';
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
      $prefix_sources_identifier = 'S';
      foreach ($documents as $document) {
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
          $field_name = IDSKS_OKHUB_SOURCE_CODE . '_' . 'object_id' . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
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
          $field_name = IDSKS_OKHUB_SOURCE_CODE . '_' . 'title' . '_' . IDSKS_DEFAULT_LANGUAGE;
          $document->setField($field_name, $source->name);
          $fields_source['title'][IDSKS_OKHUB_SOURCE_CODE] = array($source->name);
          $fields_language['title'][IDSKS_DEFAULT_LANGUAGE] = array($source->name); 
          idskshub_solr_add_hub_fields('title', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          
          // object_type
          $field_name = IDSKS_OKHUB_SOURCE_CODE . '_' . 'object_type' . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
          $field_value = 'Source';
          $document->setField($field_name, $field_value);
          // This field is not used to generate additional fields. If needed, use the same code as object_id replacing the field name.

          // Examples: contributor's name and acronym used to generate additional hub fields (search, facet) - as defined in idskshub.solrfields.inc).
          if (isset($source->contributor->name)) {
            $fields_source['contributor'][IDSKS_OKHUB_SOURCE_CODE] = array($source->contributor->name);
            $fields_language['contributor'][IDSKS_DEFAULT_LANGUAGE] = array($source->contributor->name); 
            idskshub_solr_add_hub_fields('contributor', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          }
          if (isset($source->contributor->acronym)) {
            $fields_source['acronym'][IDSKS_OKHUB_SOURCE_CODE] = array($source->contributor->acronym);
            $fields_language['acronym'][IDSKS_DEFAULT_LANGUAGE] = array($source->contributor->acronym); 
            idskshub_solr_add_hub_fields('acronym', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          }

          // language
          if (isset($source->default_language) && $language_values = idskshub_get_metadata_languages_fields($source->default_language)) {
            $language_xml_field_value = '<language>' . $language_values . '</language>';
            $indexed_name = IDSKS_OKHUB_SOURCE_CODE . '_' . 'default_language' . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
            $indexed_value = "<languageList>$language_xml_field_value</languageList>";
            $document->setField($indexed_name, $indexed_value);        
          }

          // other generic fields (see particular cases in http://data.okhub.org/fields-indexing)
          foreach ($source as $field_name => $field_value) {
            if ($field_value && !(isset($fields_types[$field_name]) && ($fields_types[$field_name] == 'ignore'))) {
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
                    $indexed_name = IDSKS_OKHUB_SOURCE_CODE . '_' . $field_name . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
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
      $item = new IdsKsItem;
      foreach ($documents as $document) {
        $metadata_languages = array();
        $has_metadata_language_field = array();
        //$fields_source = array('item_type' => array('hub' => array()), 'item_id' => array('hub' => array()));
        //$fields_language = array('item_type' => array('zz' => array()), 'item_id' => array('zz' => array()));
        $fields_source = array();
        $fields_language = array();
        $hub_fields_values = array();
        $hub_values = array();
        $available_sources = array();
        $item_id = $document->getField('item_id');
        $item->load($item_id['value']);
        if ($item->isSaved()) {
          // Generate fields based on item_id. It is already indexed, so there's no need to call setField.
          $fields_source['item_id'][IDSKS_HUB_SOURCE_CODE] = array($item_id['value']);
          $fields_language['item_id'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($item_id['value']); 
          idskshub_solr_add_hub_fields('item_id', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          // Set item type
          if (!empty($item->type->name)) { 
            $fields_source['item_type'][IDSKS_HUB_SOURCE_CODE] = array($item->type->name);
            $fields_language['item_type'][IDSKS_NOT_APPLICABLE_LANGUAGE] = array($item->type->name); 
            $document->setField('item_type', $item->type->name);
            idskshub_solr_add_hub_fields('item_type', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
          }  
          // Set literals' values
          if (!empty($item->versions_literals)) {
            $global_language_sources = array();
            foreach ($item->versions_literals as $versions_literal) {
              if (isset($versions_literal->path) && $versions_literal->path) { // See: Change to index only mapped paths???
                $tag_name = array_pop(explode('/', $versions_literal->path));
                $fields_source[$tag_name] = array();
                $fields_language[$tag_name] = array();
                if (!empty($versions_literal->literals) && !(isset($fields_types[$tag_name]) && ($fields_types[$tag_name] == 'ignore'))) {
                  foreach ($versions_literal->literals as $literal) {
                    if (isset($literal->source)) {
                      $source_id = $literal->source->getId();
                      $source_name = $literal->source->code;
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
                        if (!($literal->source->is_semantic)) {
                          $available_sources[] = $source_name;
                        }
                        $field_name =  $tag_name . '_' . $source_name;      
                        $lang = FALSE;
                        if (in_array($tag_name, $fields_translatable)) {
                          if (isset($literal->attributes)) {
                            foreach ($literal->attributes as $attribute) {
                              if ($attribute->name == IDSKS_LANGUAGE_ATTRIBUTE) {
                                if (preg_match($language_codes_regexp, $attribute->value, $matches)) {
                                  $lang = $matches[0];
                                  break;
                                }
                              }
                            }
                          }
                          // if there's no language set for the literal, we use the source's global language for the item.
                          if (!$lang) {
                            if (!isset($global_language_sources[$source_name])) {
                              $global_language_sources[$source_name] = $item->getGlobalLanguage($literal->source);
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
                    if (!$skip_literal && isset($literal->value)) {
                      $field_name = $field_name . '_' . $lang;
                      if (isset($fields_types[$tag_name])) {
                        $value = idskshub_solr_field_assign_type($literal->value, $fields_types[$tag_name]);
                      }
                      else {
                        $value = $literal->value;
                      }
                      if (isset($value)) {                   
                        idskshub_preprocess_solr_values($field_name, $value, $versions_literal->path);
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
          if (!empty($item->track_changes)) {  
            $changes = array_values($item->track_changes);
            $first_change = $changes[0];
            $latest_change = end($changes);
            if (isset($latest_change->timestamp)) {
              $timestamp = date("Y-m-d\Th:i:s\Z", $latest_change->timestamp);
              $document->setField('hub_timestamp', $timestamp);
              idskshub_solr_add_hub_fields('hub_timestamp', $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
            }
          }   
          // Add related_items' content.
          $title_search_field_values = array();
          if (!empty($item->related_items)) {
            $related_items = array();
            $related_items_xml = array();
            // We iterate twice. The first time to get the sources for which the relationships are established and the second one to add the fields.
            foreach ($item->related_items as $related_item) {
              if (isset($related_item->referred_item) && isset($related_item->source)) {
                $referred_item_id = $related_item->referred_item->getId();
                //$related_item_source_code = $related_item->source->code;
                $related_items[$referred_item_id][] = $related_item->source->code;
                if (!isset($titles_items[$referred_item_id])) {
                  $titles_items[$referred_item_id] = idskshub_get_item_display_fields_solr($related_item->referred_item, $path_title);
                }
              }
            }
            foreach ($item->related_items as $related_item) {
              if (isset($related_item->referred_item) && isset($related_item->source)) {
                $referred_item_id = $related_item->referred_item->getId();
                foreach ($titles_items[$referred_item_id] as $related_item_source_code => $title_field) {
                  // Add related items' titles. TODO: Unify with related items' values generation (below).
                  if (in_array($related_item_source_code, $related_items[$referred_item_id])) {
                    $prefix = IDSKS_HUB_SOURCE_CODE;
                  }
                  else {
                    $prefix = 'mapped';
                  }
                  $related_item_source_code = preg_replace('/_/', '', $related_item_source_code);
                  $title_field_name = $prefix . '_' . $title_field['type'] . '_' . $related_item_source_code . '_' . $title_field['language'];
                  idskshub_add_solr_not_duplicated($document, $title_field_name, $title_field['value'], $hub_solr_schema);
                  $title_search_field_name = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_search_hub_' . $title_field['language'];
                  $title_search_field_name_multilingual = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_search_hub_zx';
                  $title_facet_field_name = IDSKS_HUB_SOURCE_CODE . '_' . $title_field['type'] .'_facet_hub_zx';
                  if (!isset($title_search_field_values[$title_search_field_name])) {
                    $title_search_field_values[$title_search_field_name] = array();
                  }
                  if (!in_array($title_field['value'], $title_search_field_values[$title_search_field_name])) {
                    $title_search_field_values[$title_search_field_name][] = $title_field['value'];
                    idskshub_add_solr_not_duplicated($document, $title_search_field_name, $title_field['value'], $hub_solr_schema);
                    if (!isset($hub_fields_values[$title_field['type']])) {
                      $hub_fields_values[$title_field['type']] = array();
                    }
                    $hub_fields_values[$title_field['type']][] = $title_field['value'];
                  }
                  if (!isset($title_search_field_values[$title_search_field_name_multilingual])) {
                    $title_search_field_values[$title_search_field_name_multilingual] = array();
                  }
                  if (!in_array($title_field['value'], $title_search_field_values[$title_search_field_name_multilingual])) {
                    $title_search_field_values[$title_search_field_name_multilingual][] = $title_field['value'];
                    idskshub_add_solr_not_duplicated($document, $title_search_field_name_multilingual, $title_field['value'], $hub_solr_schema);
                  }
                  if (!isset($title_search_field_values[$title_facet_field_name])) {
                    $title_search_field_values[$title_facet_field_name] = array();
                  }
                  if (!in_array($title_field['value'], $title_search_field_values[$title_facet_field_name])) {
                    $title_search_field_values[$title_facet_field_name][] = $title_field['value'];
                    idskshub_add_solr_not_duplicated($document, $title_facet_field_name, $title_field['value'], $hub_solr_schema);
                  }
                  // Generate related_items' fields values (object_id, object_type, etc). (Needed for Eldis/BRIDGE, for instance).
                  if (in_array($related_item_source_code, $available_sources)) { // Now we are only adding the related item's fields for the sources present in the item.
                    if (isset($index_related_items_fields[$related_item_source_code][$related_item->relationship->code])) {
                      $related_item_xml = '';
                      $related_items_xml_definition = $index_related_items_fields[$related_item_source_code][$related_item->relationship->code];
                      $related_item_values = $related_item->referred_item->getValuesBySource();
                      $indexed_field_name = $related_items_xml_definition['indexed_field_name'];
                      $related_item_fields = $related_items_xml_definition['related_item_fields'];
                      if ($indexed_field_name && $related_item_fields) {
                        foreach ($related_item_fields as $related_item_field_string_paths => $related_item_field_name) {
                          $related_item_field_paths = explode('|', $related_item_field_string_paths);
                          foreach ($related_item_field_paths as $related_item_field_path) {
                            if (!empty($related_item_values[$related_item_source_code][$related_item_field_path])) {
                              $related_item_field_value = end($related_item_values[$related_item_source_code][$related_item_field_path]); // Get the latest. TODO: See languages.
                              $related_item_xml .= "<$related_item_field_name>$related_item_field_value</$related_item_field_name>";
                              break;
                            }
                          }
                        }
                        $related_item_wrappers = $related_items_xml_definition['related_item_wrappers'];
                        if ($related_item_xml) {
                          if (is_array($related_item_wrappers)) {
                            foreach ($related_item_wrappers as $related_item_wrapper) {
                              $related_item_xml = "<$related_item_wrapper>$related_item_xml</$related_item_wrapper>";
                            }
                          }
                          else {
                            $related_item_wrapper = $related_item->relationship->code;
                            $related_item_xml = "<$related_item_wrapper>$related_item_xml</$related_item_wrapper>";
                          }
                        }
                      }
                      if ($related_item_xml) {
                        $related_items_xml[$related_item_source_code][$related_item->relationship->code][] = $related_item_xml;
                      }
                    }
                  }
                }
              }
            } // end related_items loop.
            // Add the related items' generated values to the fields to index.
            foreach ($related_items_xml as $source_code_referred_item => $relationship_xmls) {
              foreach ($relationship_xmls as $relationship_code => $related_items_xml) {
                $related_items_xml_definition = $index_related_items_fields[$source_code_referred_item][$relationship_code];
                $related_items_index_xml = implode($related_items_xml);
                if (is_array($related_items_xml_definition['external_wrappers'])) {
                  foreach ($related_items_xml_definition['external_wrappers'] as $external_wrapper) {
                    $related_items_index_xml = "<$external_wrapper>$related_items_index_xml</$external_wrapper>";
                  }
                }
                else {
                  $external_wrapper = $relationship_code.'List';
                  $related_items_index_xml = "<$external_wrapper>$related_items_index_xml</$external_wrapper>";
                }
                $indexed_field_name = $index_related_items_fields[$source_code_referred_item][$relationship_code]['indexed_field_name'];
                $source_name_referred_item = preg_replace('/_/', '', $source_code_referred_item);
                $indexed_field_name .= '_' . $source_name_referred_item . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
                idskshub_preprocess_solr_values($indexed_field_name, $related_items_index_xml);
                if ($related_items_index_xml) {
                  $document->setField($indexed_field_name, $related_items_index_xml);
                  idskshub_solr_add_hub_fields($indexed_field_name, $document, $fields_language, $fields_source, $fields_translatable, $hub_fields_definitions, $hub_fields_values, $hub_solr_schema);
                }
              }
            }
          }
        }
      }
      break; // End case entity type is 'ids_ks_item'.
  } // switch
  debug($documents, 'Indexing...', TRUE);
}

// Retrieve "titles" from items. Used to add related items' titles is search and facet fields.
function idskshub_get_item_display_fields_solr($item, $field) {
  $display_fields = array();
  $type = $item->type;
  $field_name = strtolower(preg_replace('/([^A-Z])([A-Z])/', '\1_\2', $type->name));
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
    $indexed_name = IDSKS_OKHUB_SOURCE_CODE . '_' . $field_name . '_' . IDSKS_DEFAULT_LANGUAGE; // See in case these values are added in multiple/other language at some point.
  }
  else {
    $language = IDSKS_DEFAULT_LANGUAGE;
    $indexed_name = IDSKS_OKHUB_SOURCE_CODE . '_' . $field_name . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
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
    $indexed_name = IDSKS_OKHUB_SOURCE_CODE . '_' . $field_name . '_' . IDSKS_NOT_APPLICABLE_LANGUAGE;
    $indexed_value = '<' . $field_name . 'List>' . $xml . '</' . $field_name . 'List>';
    $document->addField($indexed_name, $indexed_value);
  }
}

function idskshub_get_object_xml($field_name, $obj) {
  $xml = '';
  foreach ($obj as $field => $value) { // In the future, idskshub_index_related_items_fields() could be use to indicate which properties to index.
    if (isset($value)) {
      if (is_scalar($value)) {
        $xml .= "<$field>" . $value . "</$field>";
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
      $xml .= "<$field>" . $value . "</$field>";
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

/*
function simplexml_load_file_from_url($url, $timeout = 60){
  $context = array('http' => array('timeout' => (int)$timeout));
  $data = file_get_contents($url, false, stream_context_create($context));
  if(!$data){
    trigger_error('Cannot load data from url: ' . $url, E_USER_NOTICE);
    return false;
  }
  return simplexml_load_string($data);
} 
*/
 
// Retrieve single-valued fields and field types from Solr's schema.
function idskshub_solr_schema_definitions() {
  $field_definitions = array();
  $solr_schema_url = 'http://'.IDSKS_SOLR_SERVER.'/solr/'.IDSKS_SOLR_CORE.'/admin/luke?show=schema';
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
      case 'language_array':
        if (!preg_match('/languageList/', $field_value)) {
          $field_value = preg_replace('/<list\-item>/', '<Language>', $field_value);
          $field_value = preg_replace('/<\/list\-item>/', '</Language>', $field_value);
          $field_value = "<languageList>$field_value</languageList>";
        }
        if (!(preg_match('/\[CDATA\[/', $field_value))) {
          $field_value = preg_replace('/<description>/i', '<description><![CDATA[', $field_value);
          $field_value = preg_replace('/<\/description>/i', ']]></description>', $field_value);
        }
        break;
    }
/*
    if (preg_match('/language_array/', $original_path)) {
      $field_name = 'language_array' . '_' . $source . '_' . $language;
      $str1 = 'language';
      $str2 = 'Language';
      if (!(preg_match('/\[CDATA\[/', $field_value))) {
          $field_value = preg_replace('/<description>/i', '<description><![CDATA[', $field_value);
          $field_value = preg_replace('/<\/description>/i', ']]></description>', $field_value);
      }
    }
*/
    $field_value = preg_replace('/\>\s*/', '>', $field_value);
    $field_value = preg_replace('/\s*\</', '<', $field_value);
  }
}

