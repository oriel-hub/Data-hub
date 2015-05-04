<?php
// $Id$

/**
 * @file
 *   IDS KS Hub Module interface.
 *
 * These functions implement elements of the IDS KS Hub interface.
 *
 */

// 

/* ------------------------------------------ User messages --------------------------- */

function idskshub_progress_message() {
  $active = idskshub_get('import_active');
  $message = '';
  $import_error_message = idskshub_get('import_error_message');
  if ($import_error_message) {
    $message .= '<p>' . $import_error_message . '</p>';
  }
  if ($active) {
    $message .= '<p>' . t('Import in progress...') . '</p>';
    $progress = idskshub_get('import_progress');
    if ($progress) {
      $message .= '<p>' . $progress . '</p>';
    }
    $message .= '<p>' . t('<a href="@reload">Reload this page to update</a>', array('@reload' => 'javascript:location.reload();')) . '</p>';
  }
  else {
    $import_time = idskshub_get('import_time');
    if ($import_time) {
      $message .= '<p>' . t('Latest import: ') . $import_time . '</p>';
    }
  }
  $import_error_message = idskshub_set('import_error_message', '');
  return $message;
}

function idskshub_preprocess_page(&$variables) {
  $current_page = drupal_get_path_alias();
  if ($current_page == 'imported-items') {
    drupal_set_message(idskshub_progress_message());
  }
}

/* --------------- Helper functions to build interface elements (forms, blocks) ------- */

/**
* Build and display a form defined in this module.
*/
function idskshub_form($form, $args = array()) {
  print drupal_render(drupal_get_form('idskshub_' . $form . '_form', $args));
}

/**
* Build and display entities´ management forms.
*/
function idskshub_entity_add_form($entity_type_name, $bundle_name) {
  print drupal_render(eck__entity__add($entity_type_name, $bundle_name));
}

function idskshub_form_eck__entity__delete_form_alter(&$form, &$form_state, $form_id) {
  if (isset($form['entity']['#value']) && ($entity = $form['entity']['#value'])) {
    if (isset($entity->type) && preg_match('/ids_ks_/', $entity->type)) {
      $form['#submit'][0] = 'idskshub_delete_entity_form';
    }
  }
}

// Mappings are supposed to be created with another form (idskshub_newmapping_form), but just in case.
function eck__entity__form_add_ids_ks_mapping_ids_ks_mapping_validate($form, &$form_state) {
  if ($atrs = IdsKsAttribute::getAttributesString(preg_replace('/\s*\(\d+\)+/', '', idskshub_get_field_input('field_mapping_attributes_origin', $form_state)))) {
    foreach ($atrs as $delta => $atr) {
      idskshub_set_field_reference('field_mapping_attributes_origin', $atr->getId(), $form_state, $delta);
    }
  }
  idskshub_validate_paths('field_mapping_path_origin', $form, $form_state, TRUE, TRUE);
  idskshub_validate_paths('field_mapping_path_destination', $form, $form_state, FALSE, TRUE);
}

// TODO: Implement editing form (with additional validations).
function eck__entity__form_edit_ids_ks_mapping_ids_ks_mapping_validate($form, &$form_state) {
  if ($atrs = IdsKsAttribute::getAttributesString(preg_replace('/\s*\(\d+\)+/', '', idskshub_get_field_input('field_mapping_attributes_origin', $form_state)))) {
    foreach ($atrs as $delta => $atr) {
      idskshub_set_field_reference('field_mapping_attributes_origin', $atr->getId(), $form_state, $delta);
    }
  }
  $field_mapping_action = idskshub_get_field_input('field_mapping_action', $form_state);
  if (!empty($field_mapping_action[0]['value']) && !in_array($field_mapping_action[0]['value'], array_keys(idskshub_mapping_actions()))) {
    form_set_error('field_mapping_action', t("@action is not a valid action.", array('@action' => $field_mapping_action[0]['value'])));    
  }
  idskshub_validate_paths('field_mapping_path_origin', $form, $form_state, TRUE, TRUE);
  $destination_path_required = (empty($field_mapping_action[0]['value']) || !in_array($field_mapping_action[0]['value'], array_keys(idskshub_mapping_actions())));
  idskshub_validate_paths('field_mapping_path_destination', $form, $form_state, FALSE, $destination_path_required);
}

function eck__entity__form_add_ids_ks_type_ids_ks_type_validate($form, &$form_state) {
  idskshub_validate_paths('field_type_identifiers', $form, $form_state);
  idskshub_validate_paths('field_type_description_paths', $form, $form_state);
  idskshub_validate_paths('field_type_title_paths', $form, $form_state);
}

function eck__entity__form_edit_ids_ks_type_ids_ks_type_validate($form, &$form_state) {
  idskshub_validate_paths('field_type_identifiers', $form, $form_state);
  idskshub_validate_paths('field_type_description_paths', $form, $form_state);
  idskshub_validate_paths('field_type_title_paths', $form, $form_state);
}

function eck__entity__form_add_ids_ks_format_ids_ks_format_validate($form, &$form_state) {
  idskshub_validate_slug('field_format_code', 'field_format_name', $form, $form_state);
  idskshub_validate_paths('field_format_item_paths', $form, $form_state);
  idskshub_validate_paths('field_format_type_path', $form, $form_state);
  idskshub_validate_paths('field_format_root_path', $form, $form_state);
}

function eck__entity__form_edit_ids_ks_format_ids_ks_format_validate($form, &$form_state) {
  idskshub_validate_slug('field_format_code', 'field_format_name', $form, $form_state);
  idskshub_validate_paths('field_format_item_paths', $form, $form_state);
  idskshub_validate_paths('field_format_type_path', $form, $form_state);
  idskshub_validate_paths('field_format_root_path', $form, $form_state);
}

function eck__entity__form_add_ids_ks_source_ids_ks_source_validate($form, &$form_state) {
  idskshub_validate_slug('field_source_code', 'field_source_name', $form, $form_state);
  idskshub_validate_paths('field_source_identifiers', $form, $form_state);
}

function eck__entity__form_edit_ids_ks_source_ids_ks_source_validate($form, &$form_state) {
  idskshub_validate_slug('field_source_code', 'field_source_name', $form, $form_state);
  idskshub_validate_paths('field_source_identifiers', $form, $form_state);
}

function idskshub_validate_paths($field_path, $form, &$form_state, $wildcard=FALSE, $required=FALSE) {
  if ($field_paths_form = idskshub_get_field_values($field_path, $form_state)) {
    foreach ($field_paths_form as $delta => $field_path_form) {
      if (!empty($field_path_form['value'])) {
        if (idskshub_check_path($field_path_form['value'], $wildcard)) {
          $field_path_form['value'] = strtolower(trim($field_path_form['value'], '/'));
          idskshub_set_field_value($field_path, $field_path_form['value'], $form_state, $delta);
        }
        else {
          form_set_error($field_path, t("@field is not a valid path name.", array('@field' => $field_path_form['value'])));
        }
      }
      elseif ($required) {
        form_set_error($field_path, t("The field @field can not be left empty.", array('@field' => $field_path)));
      }
    }
  }
}

function idskshub_validate_slug($field_slug, $field_orig, $form, &$form_state) {
  if ($field_slug_form = array_shift(idskshub_get_field_values($field_slug, $form_state))) {
    if (empty($field_slug_form['value'])) {
      $field_orig_form = array_shift(idskshub_get_field_values($field_orig, $form_state));
      if (!empty($field_orig_form['value'])) {
        $slug = idskshub_string_to_slug($field_orig_form['value']);
        idskshub_set_field_value($field_slug, $slug, $form_state);
      }
      else {
        form_set_error($field_orig, t("The field @field can not be left empty.", array('@field' => $field_orig)));
      }
    }
    elseif (!preg_match('/^[a-z][_a-z0-9]+$/', $field_slug_form['value'])) {
      form_set_error($field_slug, t("The field @field can only start with a lower case letter and contain lower case letters, digits and underscores.", array('@field' => $field_slug)));
    }
  }
}

function idskshub_check_path($path, $wildcard = FALSE) {
  if ($wildcard) {
    $reg = '/^(\*\/)?[a-zA-Z0-9_\-\:\/]+$/';
  }
  else {
    $reg = '/^[a-zA-Z0-9_\-\:\/]+$/';
  }
  return preg_match($reg, $path);
}

function idskshub_get_field_values($field_name, $form_state) {
//TODO: Check language support.
  return $form_state['values'][$field_name][LANGUAGE_NONE];
}

function idskshub_get_field_input($field_name, $form_state) {
//TODO: Check language support.
  return $form_state['input'][$field_name][LANGUAGE_NONE];
}

function idskshub_set_field_value($field_name, $value, &$form_state, $delta=0) {
//TODO: Check language support.
  $form_state['values'][$field_name][LANGUAGE_NONE][$delta] = array('value' => $value);
}

function idskshub_set_field_reference($field_name, $target_id, &$form_state, $delta=0) {
//TODO: Check language support.
  $form_state['values'][$field_name][LANGUAGE_NONE][$delta] = array('target_id' => $target_id);
}


/**
* Build and display a view block.
*/
function idskshub_block($block_name) {
  $block = module_invoke('views', 'block_view', $block_name);
  print render($block['content']);
}

/* ----------------------------------- Entity display --------------------------------- */

// Returns an item's displayable fields (title, description), based on its type.
function idskshub_item_display_field($id, $field, $source = NULL, $lang = IDSKS_DEFAULT_LANGUAGE) {
  if ($source) {
    $source_code = $source->code;
  }
  else {
    $source_code = '';
  }
  $item = new IdsKsItem();
  $item->load($id);
  $display = $item->getDisplayFieldValue($field, $lang, $source_code);
  return $display;
}

function idskshub_display_search_results ($data, $field, $source = '', $lang = '') {
  $data = $data->_entity_properties;
  //print_r($data);
  if (!$source) {
    $source = 'hub';
  }
  if (!$lang) {
    $lang = IDSKS_UNDEFINED_LANGUAGE_INDEX;
  }
  $field_name = $field . '_' . $source . '_' . $lang;
  if (isset($data[$field_name])) {
    echo $data[$field_name];
  }
}

// Returns an item's list of sources.
function idskshub_item_display_sources($id) {
  $sources = idskshub_get_item_sources($id);
  return implode(',', $sources);
}

function idskshub_print_mapping_entity($entity) {
  $mapping = new IdsKsMapping();
  $mapping->populate($entity);
  return $mapping->getLabel();
}

function idskshub_mappings_strings($mappings_ids) {
  $mapping_strings = array();
  $mapping = new IdsKsMapping();
  foreach ($mappings as $mapping_id) {
    $mapping->load($mapping_id);
    $mapping_strings[] = $mapping->getLabel();
  }
  return array_unique($mapping_strings);
}

// Builds an array of entities of a given type, indexed by id.
// There is a problem when calling entity_load more than once, so we are not using this now (http://drupal.org/node/1542824).
function idskshub_array_entities($entity_type, $sort = FALSE) {
  $results = array();
  $entities = entity_load($entity_type);
  foreach ($entities as $id => $entity) {
    $results[$id] = idskshub_entity_label($entity, $entity_type);
  }
  return $results;
}

function idskshub_sources_options() {
  $options = array();
  $sources_info = IdsKsObject::getArrayObjects('IdsKsSource' , array('name', 'contributor', 'is_semantic'));
  foreach ($sources_info as $id => $source_info) {
    if (empty($source_info['is_semantic'])) {
      if (isset($source_info['name'])) {
        $options[$id] = $source_info['name'];
      }
      else {
        if (isset($source_info['contributor'])) {
          if (isset($source_info['contributor']->acronym)) {
            $options[$id] = $source_info['contributor']->acronym;
          }
          elseif (isset($source_info['contributor']->name)) {
            $options[$id] = $source_info['contributor']->name;
          }
          else {
            $options[$id] = $id;
          }
        }
      }
    }
  }
  asort($options);
  return $options;
}

/* ---------------------------------------- Forms ------------------------------------- */

function idskshub_display_field_types() {
  $field_types = idskshub_get('field_types');
  if (!empty($field_types)) {
    asort($field_types);
    echo "<ul>";
    foreach ($field_types as $field_name => $field_type) {
      echo "<li> $field_name : $field_type";
    }
    echo "</ul>";
  }
}

function idskshub_field_types_form($form, $form_state) {
  // Field name.
  $form['field_types']['field_name'] = array(
    '#title' => t('Field name'),
    '#description' => t('Name of the field that requires type validation when indexing. For instance: <i>date_updated</i>.'),
    '#weight' => '1',
    '#type' => 'textfield',
    '#maxlength'=> 255,
    '#size'=> 60,
    '#required' => '1',
  );
  // Type of fields.
  $form['field_types']['field_type'] = array(
    '#title' => t('Field type'),
    '#description' => t('Type of the field. Not necessary for string or text fields.'),
    '#type' => 'select',
    '#options' => array(
            'date' => 'date',
            'int' => 'integer',
            'ignore' => 'ignore',
            'none' => 'not defined',
          ),
    '#multiple' => '0',
    '#weight' => '2',
  );
 $form['actions']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Set field type'),
  );
  return $form;
}

/**
* Implements hook_form_validate().
*/
function idskshub_field_types_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $field_name = $values['field_name'];
  if (!preg_match('/^[_a-zA-Z0-9]+$/', $field_name)) {
    form_set_error('field_name', t("The field name can only contain alphanumeric characters and underscores."));
  }
}

/**
* Implements hook_form_submit().
*/
function idskshub_field_types_form_submit($form, &$form_state) {
  $error = FALSE;
  $values = $form_state['values'];
  $field_name = $values['field_name'];
  $field_type = $values['field_type'];
  $field_types = idskshub_get('field_types');
  if ($field_type == 'none') {
    if (isset($field_types[$field_name])) {
      unset($field_types[$field_name]);
    }
  }
  else {
    $field_types[$field_name] = $field_type;
  }
  idskshub_set('field_types', $field_types);
}

function idskshub_display_translatable_fields() {
  $translatable_fields = idskshub_get('translatable_fields');
  if (!empty($translatable_fields)) {
    sort($translatable_fields);
    echo "<ul>";
    foreach ($translatable_fields as $field_name) {
      echo "<li> $field_name";
    }
    echo "</ul>";
  }
}

function idskshub_translatable_fields_form($form, $form_state) {
  // Field name.
  $form['fields_translatable']['field_name'] = array(
    '#title' => t('Field name'),
    '#description' => t('Name of the field. For instance: <i>description</i>.'),
    '#weight' => '1',
    '#type' => 'textfield',
    '#maxlength'=> 255,
    '#size'=> 60,
    '#required' => '1',
  );
  // Type of fields.
  $form['fields_translatable']['field_translatable'] = array(
    '#title' => t('Is translatable'),
    '#description' => t('Indicate whether the field is translatable or not. By default, fields are considered not to be translatable.'),
    '#type' => 'select',
    '#options' => array(
            'yes' => 'Translatable',
            'no' => 'Not translatable',
          ),
    '#multiple' => '0',
    '#weight' => '2',
  );
 $form['actions']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Set field'),
  );
  return $form;
}

function idskshub_display_single_fields() {
  $single_fields = idskshub_get('single_fields');
  if (!empty($single_fields)) {
    sort($single_fields);
    echo "<ul>";
    foreach ($single_fields as $field_name) {
      echo "<li> $field_name";
    }
    echo "</ul>";
  }
}

function idskshub_single_fields_form($form, $form_state) {
  // Field name.
  $form['fields_single']['field_name'] = array(
    '#title' => t('Field name'),
    '#description' => t('Name of the field. For instance: <i>object_id</i>.'),
    '#weight' => '1',
    '#type' => 'textfield',
    '#maxlength'=> 255,
    '#size'=> 60,
    '#required' => '1',
  );
  // Type of fields.
  $form['fields_single']['field_single'] = array(
    '#title' => t('Is single'),
    '#description' => t('Indicate whether the field is single-valued or not. By default, fields are considered not to be single-valued.'),
    '#type' => 'select',
    '#options' => array(
            'yes' => 'Single-valued',
            'no' => 'Not singled-valued',
          ),
    '#multiple' => '0',
    '#weight' => '2',
  );
 $form['actions']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Set field'),
  );
  return $form;
}

/**
* Implements hook_form_submit().
*/
function idskshub_single_fields_form_submit($form, &$form_state) {
  $error = FALSE;
  $values = $form_state['values'];
  $field_name = $values['field_name'];
  $field_single = $values['field_single'];
  $single_fields = idskshub_get('single_fields');
  if ($field_single == 'no') {
    $key = array_search($field_name, $single_fields);
    if ($key !== false) {
      unset($single_fields[$key]);
      $single_fields = array_values($single_fields);
    }
  }
  else {
    $single_fields[] = $field_name;
  }
  idskshub_set('single_fields', $single_fields);
}

/**
* Implements hook_form_validate().
*/
function idskshub_single_fields_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $field_name = $values['field_name'];
  if (!preg_match('/^[_a-zA-Z0-9]+$/', $field_name)) {
    form_set_error('field_name', t("The field name can only contain alphanumeric characters and underscores."));
  }
}

/**
* Implements hook_form_submit().
*/
function idskshub_translatable_fields_form_submit($form, &$form_state) {
  $error = FALSE;
  $values = $form_state['values'];
  $field_name = $values['field_name'];
  $field_translatable = $values['field_translatable'];
  $translatable_fields = idskshub_get('translatable_fields');
  if ($field_translatable == 'no') {
    $key = array_search($field_name, $translatable_fields);
    if ($key !== false) {
      unset($translatable_fields[$key]);
      $translatable_fields = array_values($translatable_fields);
    }
  }
  else {
    $translatable_fields[] = $field_name;
  }
  idskshub_set('translatable_fields', $translatable_fields);
}

/**
* Implements hook_form_validate().
*/
function idskshub_translatable_fields_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $field_name = $values['field_name'];
  if (!preg_match('/^[_a-zA-Z0-9]+$/', $field_name)) {
    form_set_error('field_name', t("The field name can only contain alphanumeric characters and underscores."));
  }
}

/* ----------------------------- Fetch / upload file to import ------------------------ */

// Currently supported types. // TODO: Get them from the database.
function idskshub_valid_types() {
/*
  $valid_types = array(
    'data' => t('Determine from the data format'),
  );
*/
  $valid_types = idskshub_get_types_names();
  return $valid_types;
}

/**
* Definition of the content mappings form.
*/
function idskshub_fetcher_import_sources(&$form, $sources) {
  $formats = idskshub_get_formats_codes();
  $types = idskshub_valid_types();
  foreach (array_keys($sources) as $source_id) {
    if ($source_id) {
      $feeds_ids = IdsKsFeed::getParentsIds(array('source' => $source_id));
      $field_name_origin = 'import_origin_' . $source_id;
      if (!empty($feeds_ids)) {
        $options_feeds = array();
        foreach ($feeds_ids as $feed_id) {
          $feed = new IdsKsFeed();
          $feed->load($feed_id);
          if ($feed->isSaved()) {
            $options_feeds[$feed->getId()] = $feed->name;
          } 
        }
        $field_name_feed = 'feed_id_' . $source_id;
        $form['fetch_file'][$field_name_feed] = array(
          '#title' => t('Feed'),
          '#required' => '0',
          '#multiple' => '0',
          '#description' => t('The feed to import from.'),
          '#type' => 'select',
          '#options' => $options_feeds,
          '#weight' => '25',
          '#states' => array(
            'visible' => array(
              ':input[name="source_id"]' => array('value' => $source_id),
              ':input[name=' . $field_name_origin. ']' => array('value' => 'feed'),
              ),
            ),
          );
        $options_origin = array('feed' => t('Import from one of the source\'s feeds'));
        $default_value = 'feed';
      }
      else {
        $options_origin = array();
        $default_value = 'url';
      }
      $options_origin += array(
          'url' => t('Retrieve content from another URL'),
          'file' => t('Upload a local file'),
          ); 
      // Source to use as reference.
      $form['fetch_file'][$field_name_origin] = array(
        '#title' => t('Import origin'),
        '#required' => '0',
        '#multiple' => '0',
        '#description' => t('Indicate the origin to retrieve the content from.'),
        '#default_value' => $default_value,
        '#type' => 'radios',
        '#weight' => '23',
        '#options' => $options_origin,
        '#states' => array(
          'visible' => array(
            ':input[name="source_id"]' => array('value' => $source_id),
          ),
        ),
      );
      // Alternative URL.
      $field_name_url = 'url_' . $source_id;
      $form['fetch_file'][$field_name_url] = array(
        '#title' => t('URL'),
        '#required' => '0',
        '#multiple' => '0',
        '#description' => t('URL of the feed to retrieve.'),
        '#type' => 'textfield',
        '#size' => 120, 
        '#maxlength' => 400, 
        '#weight' => '25',
        '#states' => array(
          'visible' => array(
            ':input[name="source_id"]' => array('value' => $source_id),
            ':input[name=' . $field_name_origin. ']' => array('value' => 'url'),
          ),
        ),
      );
      // Upload file
      $field_name_file = 'file_' . $source_id;
      $form['fetch_file'][$field_name_file] = array(
        '#title' => t('Select file'),
        '#description' => t('Upload file with the definition of items to import into the IDS KS data hub. Valid file extensions: xml rss csv txt'),
        '#type' => 'managed_file',
        '#upload_validators' => array('file_validate_extensions' => array('xml rss csv txt')),
        '#size' => '30',
        '#weight' => '25',
        '#states' => array(
          'visible' => array(
            ':input[name="source_id"]' => array('value' => $source_id),
            ':input[name=' . $field_name_origin. ']' => array('value' => 'file'),
          ),
        ),
      );
      // File format (only if not retrieving from existing feed)
      $field_name_format = 'file_format_' . $source_id;
      $form['fetch_file'][$field_name_format] = array(
        '#title' => t('File format'),
        '#description' => t('Format of the file to import.'),
        '#type' => 'select',
        '#default_value' => IDSKS_DEFAULT_FORMAT,
        '#options' => $formats,
        '#multiple' => '0',
        '#weight' => '28',
        '#states' => array(
          'visible' => array(
            ':input[name="source_id"]' => array('value' => $source_id),
            ':input[name=' . $field_name_origin. ']' => array(array('value' => 'url'), array('value' => 'file')),
            ),
          'required' => array(
            ':input[name="source_id"]' => array('value' => $source_id),
            ':input[name=' . $field_name_origin. ']' => array(array('value' => 'url'), array('value' => 'file')),
            ),
          ),
      );
      // Type of items to create.
      $field_name_type = 'items_type_' . $source_id;
      $form['fetch_file'][$field_name_type] = array(
        '#title' => t('Items type'),
        '#description' => t('Type of items to import.'),
        '#default_value' => IDSKS_DEFAULT_TYPE_ITEMS,
        '#type' => 'select',
        '#options' => $types,
        '#multiple' => '0',
        '#weight' => '30',
        '#states' => array(
          'visible' => array(
            ':input[name="source_id"]' => array('value' => $source_id),
            ':input[name=' . $field_name_origin. ']' => array(array('value' => 'url'), array('value' => 'file')),
            ),
          'required' => array(
            ':input[name="source_id"]' => array('value' => $source_id),
            ':input[name=' . $field_name_origin. ']' => array(array('value' => 'url'), array('value' => 'file')),
            ),
          ),
      );
    }
  }
}

/**
* Definition of the fetcher form.
*/
function idskshub_fetcher_form($form, $form_state) {
  $sources = idskshub_sources_options();
  //$sources[0] = '- Select source -';
  $semantic_services = idskshub_get_semantic_services();
  // Source to use as reference.
  $form['fetch_file']['source_id'] = array(
    '#title' => t('Source'),
    //'#default_value' =>  '0',
    '#required' => '1',
    '#multiple' => '0',
    '#description' => t('The source to import from.'),
    '#type' => 'select',
    '#options' => $sources,
    '#weight' => '20',
  );
  // Feeds and origins
  idskshub_fetcher_import_sources($form, $sources);
  // Advanced options
  $form['fetch_file']['advanced'] = array(
    '#type' => 'checkboxes',
    '#options' => array('show' => 'Show advanced options'),
    '#multiple' => '0',
    '#weight' => '45',
  );
  $form['fetch_file']['semantic_augmentation'] = array(
    '#title' => t('Semantic augmentation'),
    '#description' => t('Use automatic semantic augmentation services. If not selected, imported content will be augmented automatically afterwards (via cron). (Recommended)'),
    //'#default_value' => array_keys($semantic_services),
    '#type' => 'checkboxes',
    '#options' => $semantic_services,
    '#weight' => '50',
    '#states' => array(
      'visible' => array(
        ':input[name="advanced[show]"]' => array('checked' => TRUE),
      ),
    ),
  );
/*
  $form['fetch_file']['existing_items'] = array(
    '#title' => t('Existing items'),
    '#description' => t('Indicate what to do with existing items from the source being imported.'),
    '#default_value' => 'update_existing',
    '#type' => 'radios',
    '#options' => array(
      'update_existing' => t('Update existing items with values from the source being imported'),
      'skip_existing' => t('Skip existing items from the source being imported (leaving their current values in the hub)'),
    ),
    '#weight' => '55',
    '#states' => array(
      'visible' => array(
        ':input[name="advanced[show]"]' => array('checked' => TRUE),
      ),
    ),
  );
*/
  $form['fetch_file']['update_mode'] = array(
    '#title' => t('Update mode'),
    '#description' => t('When updating an existing item, indicate how to process the potentially new content from the source being imported.'),
    '#default_value' => 'incremental',
    '#type' => 'radios',
    '#options' => array(
      'incremental' => t('Incrementally add new content from the source to the one present in existing items') . '<small><i> (' . t('partial updates') . ')</i></small>',
      'replace' => t('Replace all content from the source in existing items with the new one being imported') . '<small><i> (' . t('total updates') . ')</i></small>',
    ),
    '#weight' => '60',
    '#states' => array(
      'visible' => array(
        //':input[name="existing_items"]' => array('value' => 'update_existing'),
        ':input[name="advanced[show]"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['fetch_file']['allow_multiple'] = array(
    '#title' => t('Existing values on update'),
    '#description' => t('When adding content from the source to the one already existing, please indicate how to process fields that are already present in the item and have values (for example: authors)'),
    '#default_value' => 'replace',
    '#type' => 'radios',
    '#options' => array(
      'replace' => t('Replace existing values with the new ones from the source being imported'),
      'allow_multiple' => t('Keep existing values from the source and add new ones, without duplicates'),
    ),
    '#states' => array(
      'visible' => array(
        //':input[name="existing_items"]' => array('value' => 'update_existing'),
        ':input[name="update_mode"]' => array('value' => 'incremental'),
        ':input[name="advanced[show]"]' => array('checked' => TRUE),
      ),
    ),
    '#weight' => '70',
  );
  $form['fetch_file']['import_mode'] = array(
    '#title' => t('Import mode'),
    '#description' => t('Indicate if the import should be run in background (non-interactive) process.<br>
      Large imports will be switched to background mode by default.'),
    '#default_value' => 'default',
    '#type' => 'radios',
    '#options' => array(
      'default' => t('Default'),
      'background' => t('Background mode'),
      'interactive' => t('Force interactive'),
    ),
    '#weight' => '80',
    '#states' => array(
      'visible' => array(
        ':input[name="advanced[show]"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['fetch_file']['index_imported'] = array(
    '#title' => t('Index imported items'),
    '#description' => t('Add new items to the Solr index immediately. If not selected, imported content will be indexed automatically afterwards (via cron). (Recommended)'),
    '#default_value' => array(),
    '#type' => 'checkboxes',
    '#options' => array('index' => 'Index imported items'),
    '#weight' => '90',
    '#states' => array(
      'visible' => array(
        ':input[name="advanced[show]"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['fetch_file']['force_import'] = array(
    '#title' => t('Force importing of unchanged items'),
    '#description' => t('If checked, unchanged items will not be skipped.'),
    '#default_value' => array(),
    '#type' => 'checkboxes',
    '#options' => array('force' => 'Do not skip unchanged items'),
    '#weight' => '100',
    '#states' => array(
      'visible' => array(
        ':input[name="advanced[show]"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['actions']['next'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
  );
  return $form;
}

/**
* Implements hook_form_validate().
*/
function idskshub_fetcher_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $source_id = $values['source_id'];
  $import_origin = $form_state['values']['import_origin_' . $source_id];
  if ($import_origin == 'url') {
    if (!valid_url($form_state['values']['url_' . $source_id])) {
      form_set_error('url_' . $source_id, t('The entered URL is not valid.'));
    }
  }
  elseif ($import_origin == 'file') {
    if (!$form_state['values']['file_' . $source_id]) {
      form_set_error('file', t('The file was not uploaded.'));
    }
  }
}

/**
* Implements hook_form_submit().
*/
function idskshub_fetcher_form_submit($form, &$form_state) {
  $error = FALSE;
  $feed_id = 0;
  $url = '';
  $values = $form_state['values'];
  $source_id = $values['source_id'];
  $type_name = $values['items_type_' . $source_id];
  $format_code = $values['file_format_' . $source_id];
  if ($values['file_' . $source_id]) {
    $fid = $values['file_' . $source_id];
  }
  else {
    $fid = 0;
    if ($values['url_' . $source_id]) {
      $url = $values['url_' . $source_id];
    }
    elseif ('feed_id_' . $source_id) {
      $feed_id = $values['feed_id_' . $source_id];
      $type_name = 'feed';
      $format_code = '';
    }
    else {
      $feed_id = 0;
    }
  }
  $mode = $values['import_mode'];
  $semantic = $values['semantic_augmentation'];
  //$existing_items = $values['existing_items'];
  $existing_items = 'update_existing';
  $update_mode = $values['update_mode'];
  $allow_multiple = $values['allow_multiple'];
  $index_imported = $values['index_imported'];
  $force_import = $values['force_import'];
  IdsKsFetcher::fetchFile($fid, $url, $source_id, $feed_id, $format_code, $type_name, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $index_imported, $force_import);
}

/*----------------------- Notifications ---------------------------*/



function idskshub_notifications_form($form, &$form_state) {
  $roles = user_roles(TRUE);
  $roles[0] = '- None -';
  ksort($roles);
  $users = array();
  foreach (entity_load('user') as $uid => $user) {
    $users[$uid] = $user->name;
  }
  $users[0] = '- None -';
  $sources = idskshub_sources_options();
  $sources[0] = '- All sources -';
  asort($sources);
  $types = idskshub_get_types_ids();
  $types[0] = '- All types -';
  asort($types);
  $notifications_items = idskshub_get('notify_items');
  if (empty($form_state['num_imports'])) { // first time the form is loaded.
    if (!empty($notifications_items)) {
      $form_state['num_imports'] = count($notifications_items);
    }
    else {
      $form_state['num_imports'] = 1;
    }
  }
  $form['#tree'] = TRUE;
  $form['items_imported'] = array(
    '#type' => 'fieldset',
    '#title' => t('Items imported or updated'),    
    '#description' => t('Please indicate who should be notified when items from these sources/types are created, updated or deleted.') . '<br>' . 
                      t('The selected usernames will be added to the list of recipients - in addition to the users that are included in the selected roles.'),
    '#prefix' => '<div id="items-imported-wrapper">',
    '#suffix' => '</div>',
  );
  for ($i = 0; $i < $form_state['num_imports']; $i++) {
    if (isset($form_state['values']['items_imported'][$i]['delete'])) {
      $deleted = $form_state['values']['items_imported'][$i]['delete'];
    }
    else {
      $deleted = FALSE;
    }
    $form['items_imported'][$i] = array(
      '#access' => !$deleted,
      '#type' => 'fieldset',
      '#title' => t('Indicate users/roles to notify when items from these sources/types are created, updated or deleted.'),    
      '#prefix' => '<div id="items-imported-wrapper">',
      '#suffix' => '</div>',
      
    );
    $form['items_imported'][$i]['delete'] = array(
      '#default_value' => array(),
      '#type' => 'checkbox',
      '#prefix' => '<div style="float:left; margin-right: 30px;">',
      '#suffix' => '</div>',
    );
    if (isset($notifications_items[$i]['sources'])) {
      $default_sources = $notifications_items[$i]['sources'];
    }
    else {
      $default_sources = 0;
    }
    $form['items_imported'][$i]['sources'] = array(
      '#title' => t('Sources'),
      '#type' => 'select',
      '#multiple' => '1',
      '#default_value' => $default_sources,
      '#options' => $sources,
      '#prefix' => '<div style="float:left; margin-right: 15px;">',
      '#suffix' => '</div>',
    );
    if (isset($notifications_items[$i]['types'])) {
      $default_types = $notifications_items[$i]['types'];
    }
    else {
      $default_types = 0;
    }
    $form['items_imported'][$i]['types'] = array(
      '#title' => t('Types'),
      '#type' => 'select',
      '#options' => $types,
      '#multiple' => '1',
      '#default_value' => $default_types,
      '#prefix' => '<div style="float:left; margin-right: 15px;">',
      '#suffix' => '</div>',
    );
    if (isset($notifications_items[$i]['roles'])) {
      $default_roles = $notifications_items[$i]['roles'];
    }
    else {
      $default_roles = 0;
    }
    $form['items_imported'][$i]['roles'] = array(
      '#title' => t('Roles'),
      '#type' => 'select',
      '#options' => $roles,
      '#multiple' => '1',
      '#default_value' => $default_roles,
      '#prefix' => '<div style="float:left; margin-right: 15px;">',
      '#suffix' => '</div>',
    );
    if (isset($notifications_items[$i]['users'])) {
      $default_users = $notifications_items[$i]['users'];
    }
    else {
      $default_users = 0;
    }
    $form['items_imported'][$i]['users'] = array(
      '#title' => t('Users'),
      '#type' => 'select',
      '#options' => $users,
      '#multiple' => '1',
      '#default_value' => $default_users,
      '#prefix' => '<div style="float:left; margin-right: 15px;">',
      '#suffix' => '</div>',
    );
/*
    if (isset($notifications_items[$i]['when'])) {
      $default_when = $notifications_items[$i]['when'];
    }
    else {
      $default_when = 'daily';
    }
    $form['items_imported'][$i]['when'] = array(
      '#title' => t('When'),
      '#type' => 'select',
      '#options' => array('daily' => 'On a daily basis', 'import_finished' => 'Each time an import process is completed'),
      '#default_value' => $default_when,
      '#prefix' => '<div style="float:left; margin-right: 15px;">',
      '#suffix' => '</div>',
    ); 
*/
  }
  $form['add_one'] = array(
    '#type' => 'submit',
    '#value' => t('Add notification'),
    '#submit' => array('idskshub_notifications_add_more'),    
    '#ajax' => array(
      'callback' => 'idskshub_notifications_callback',
      'wrapper' => 'items-imported-wrapper',
    ),
    '#prefix' => '<div style="float:left; margin-right: 15px;">',
    '#suffix' => '</div>',
  );
  if ($form_state['num_imports'] > 0) {
    $form['remove_one'] = array(
      '#type' => 'submit',
      '#value' => t('Remove selected'),
      '#submit' => array('idskshub_notifications_remove_selected'),
      '#ajax' => array(
        'callback' => 'idskshub_notifications_callback',
        'wrapper' => 'items-imported-wrapper',
      ),
      '#prefix' => '<div style="float:left; margin-right: 15px;">',
      '#suffix' => '</div>',
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save notifications'),
    '#prefix' => '<div style="margin-right: 15px;">',
    '#suffix' => '</div>',
  );
  return $form;
}

function idskshub_notifications_add_more($form, &$form_state) {
  $form_state['num_imports']++;
  $form_state['rebuild'] = TRUE;
}

function idskshub_notifications_remove_selected($form, &$form_state) {
  idskshub_save_notifications_items($form_state['values']['items_imported']);
  $form_state['rebuild'] = TRUE;
}

function idskshub_notifications_callback($form, $form_state) {
  return $form['items_imported'];
}

function idskshub_notifications_form_submit($form, &$form_state) {
  idskshub_save_notifications_items($form_state['values']['items_imported']);
}

function idskshub_save_notifications_items($items_imported) {
  $notifications_items = array();
  foreach ($items_imported as $key => $notification) {
    if (!empty($notification)) {
      if (!$notification['delete'] && (($notification['sources'] && $notification['sources'] !== array('0')) || ($notification['types'] && $notification['types'] !== array('0')) || ($notification['roles'] && $notification['roles'] !== array('0')) || ($notification['users'] && $notification['users'] !== array('0')))) {
        $notifications_items[] = $notification;
      }
    }
  }
  if (empty($notifications_items)) {
    idskshub_del('notify_items');
  }
  else {
    idskshub_set('notify_items', $notifications_items);
  }
}

/**
* Definition of the content mappings forms.
*/

function idskshub_content_mapping_identifiers_fields(&$form, $identifiers_paths, $info, $use_types_identifiers, $num_item, $type) {
  $group_item = 'item' . $num_item;
  $group_identifiers = 'identifiers' . $num_item;
  foreach ($use_types_identifiers as $source_id) {
    $hide_for_sources[] = array('value' => $source_id);
  }
  foreach ($identifiers_paths as $id_source_or_type => $identifiers) {
    if (!empty($identifiers)) {
      switch ($type) {
        case 'type':
          $visibility = array(
            ':input[name="type"]' => array('value' => $id_source_or_type),
            ':input[name="source' . $num_item . '"]' => $hide_for_sources,
            );
          break;
        case 'source':
          if (!in_array($id_source_or_type, $use_types_identifiers)) {
            $visibility = array(
              ':input[name="source' . $num_item . '"]' => array('value' => $id_source_or_type),
              );
          }
          break;
      }
      $form[$group_item][$group_identifiers]['identifiers_' . $type . '_' . $id_source_or_type] = array(
        '#type' => 'hidden',
        '#value' => implode(',', $identifiers),
      );
      $compound = $info[$id_source_or_type]['compound_key'];
      if (!$compound) {
        $identifier_field_name = 'identifier_' . $num_item . '---' . $type . '---' . $id_source_or_type;
  /*
        $disabled = array(
          ':input[name="' . $identifier_field_name . '"]' => array('value' => 0),
          );
  */
        $required = array(
          ':input[name="' . $identifier_field_name . '"]' => array('empty' => FALSE),
          );
        $form[$group_item][$group_identifiers][$identifier_field_name] = array(
          '#title' => t('Identified by'),
          '#multiple' => '0',
          '#type' => 'select',
          '#options' => $identifiers,
          '#default_value' => 0,
          '#prefix' => '<div class="fields-content-mappings-left">',
          '#suffix' => '</div>', 
          '#required' => FALSE,
          '#states' => array(
            'visible' => $visibility,
            //'required' => $visibility,
            ),
        );
        $value_field_name = 'value_' . $num_item . '---' . $type . '---' . $id_source_or_type;
        $form[$group_item][$group_identifiers][$value_field_name] = array(
          '#title' => t('Value'),
          '#multiple' => '0',
          '#type' => 'textfield',
          '#size' => 30, 
          '#maxlength' => 400, 
          '#prefix' => '<div class="fields-content-mappings-right">',
          '#suffix' => '</div>', 
          '#required' => FALSE,
          '#states' => array(
              'visible' => $visibility,
              //'disabled' => $disabled,
              //'required' => $visibility,
            ),
        );
      }
      else {
        foreach ($identifiers as $identifier_key => $identifier_path) {
          $value_field_name = 'value_' . $num_item . '---' . $type . '---' . $id_source_or_type . '---' . $identifier_key;
          $form[$group_item][$group_identifiers][$value_field_name] = array(
            '#title' => $identifier_path,
            '#multiple' => '0',
            '#type' => 'textfield',
            '#size' => 40, 
            '#maxlength' => 400, 
            '#required' => FALSE,
            '#states' => array(
                'visible' => $visibility,
              ),
          );
        }
      }
    }
  }
}

// Content-mapping form.
function idskshub_content_mapping_form($form, $form_state) {
  $identifiers_sources = array();
  $identifiers_types = array();
  $all_identifiers_sources = idskshub_flatten_array(IdsKsSource::getFieldValues('identifiers'));
  $all_identifiers_types = idskshub_flatten_array(IdsKsType::getFieldValues('identifiers'));
  $all_identifiers = array_values(array_unique(array_merge($all_identifiers_sources, $all_identifiers_types)));
  sort($all_identifiers);
  $types_have_identifiers = TRUE;
  $types = array();
  $types_info = IdsKsObject::getArrayObjects('IdsKsType' , array('name', 'identifiers', 'compound_key'));
  $default_source = idskshub_hub_source();
  $use_types_identifiers = array();
  foreach ($types_info as $type_id => $type_info) {
    if ($type_info['name'] !== IDSKS_GENERIC_TYPE_NAME) {
      $types[$type_id] = $type_info['name'];
      $identifiers_types[$type_id] = array();
      if (empty($type_info['identifiers'])) {
        $type_info['identifiers'] = $all_identifiers;
      }
      foreach ($type_info['identifiers'] as $identifier) {
        $identifiers_types[$type_id][] = $identifier;
      }
    }
  }
  asort($types);
  $sources_info = IdsKsObject::getArrayObjects('IdsKsSource' , array('name', 'identifiers', 'compound_key'));
  foreach ($sources_info as $source_id => $source_info) {
    $sources[$source_id] = $source_info['name'];
    $identifiers_sources[$source_id] = array();
    if (!empty($source_info['identifiers'])) {
      foreach ($source_info['identifiers'] as $identifier) {
        $identifiers_sources[$source_id][] = $identifier;
      }
    }
    else {
      $use_types_identifiers[] = $source_id;
    }
  }
  asort($sources);
  // Type of items to create.
  $form['type'] = array(
    '#title' => t('Items type') . ' (<a href="' . url('types') . '">' . t('view types') . '</a>)',
    '#type' => 'select',
    '#options' => $types,
    '#multiple' => '0',
    '#weight' => 10,
  );

  // Sources id of sources' that don't have identifiers defined.
  $form['use_type_identifiers'] = array(
    '#type' => 'hidden',
    '#value' => implode(',', $use_types_identifiers),
  );

  // First item
  $form['item1'] = array(
    '#type' => 'fieldset', 
    '#title' => t('First item'), 
    '#weight' => 15, 
    '#collapsible' => FALSE, 
    //'#collapsed' => FALSE,
    '#attributes' => array(
      'class' => array('form-content-mappings-left'),
    ),
  );
  // Source for content 1.
  $form['item1']['source1'] = array(
    '#title' => t('Source') . ' (<a href="' . url('sources') . '">' . t('view sources') . '</a>)',
    '#multiple' => '0',
    '#type' => 'select',
    '#options' => $sources,
    '#weight' => 20,
    '#default_value' => $default_source->getId(),
    '#description' => t('If there are no identifiers defined for the source, the type\'s identifiers are used.'),
  );
  $form['item1']['identifiers1'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Identifiers'),
    '#collapsible' => FALSE, 
    '#weight' => 30,
    );
  idskshub_content_mapping_identifiers_fields($form, $identifiers_sources, $sources_info, $use_types_identifiers, '1', 'source');
  idskshub_content_mapping_identifiers_fields($form, $identifiers_types, $types_info, $use_types_identifiers, '1', 'type');
  // Second item
  $form['item2'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Second item'), 
    '#weight' => 50, 
    '#collapsible' => FALSE, 
    '#attributes' => array(
      'class' => array('form-content-mappings-right'),
    ),
  );
  // Source for content 2.
  $form['item2']['source2'] = array(
    '#title' => t('Source') . ' (<a href="' . url('sources') . '">' . t('view sources') . '</a>)',
    '#multiple' => '0',
    '#type' => 'select',
    '#options' => $sources,
    '#weight' => 60,
    '#default_value' => $default_source->getId(),
    '#description' => t('If there are no identifiers defined for the source, the type\'s identifiers are used.'),
  );
  $form['item2']['identifiers2'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Identifiers'),
    '#collapsible' => FALSE, 
    '#weight' => 70,
    );
  idskshub_content_mapping_identifiers_fields($form, $identifiers_sources, $sources_info, $use_types_identifiers, '2', 'source');
  idskshub_content_mapping_identifiers_fields($form, $identifiers_types, $types_info, $use_types_identifiers, '2', 'type');
  $form['actions']['next'] = array(
    '#type' => 'submit',
    '#value' => t('Create mapping'),
    '#weight' => 100,
    '#prefix' => '<div style="float:right;">',
    '#suffix' => '</div>', 
  );
  return $form;
}

/**
* Implements hook_form_validate().
*/
function idskshub_content_mapping_form_validate($form, &$form_state) {
  $form_values = $form_state['values'];
  $field_values = array();
  $identifiers = array();
  $use_type_identifiers = explode(',', $form_values['use_type_identifiers']);
  foreach ($form_values as $input => $value) {
    if ($value !== '') { //could be false or 0.
      if ($field = explode('---', $input)) {
        $field_name = $field[0];
        if (preg_match('/value_(\d)/', $field_name, $match)) {
          $num_item = $match[1];
          $type_or_source = $field[1];
          $id_type_or_source = $field[2];
          if (isset($field[3])) { //compound
            $key_identifier = $field[3];
            $compound[$num_item][$type_or_source][$id_type_or_source] = TRUE;
          }
          else { //not compound
            $key_identifier = $form_values["identifier_$num_item---$type_or_source---$id_type_or_source"];
            $compound[$num_item][$type_or_source][$id_type_or_source] = FALSE;
          }          
          $field_values[$num_item][$type_or_source][$id_type_or_source][$key_identifier] = $value;
        }
      }
    }
  }
  if ($field_values) {
    foreach (array(1,2) as $num_item) {
      $type_source[$num_item] = (!in_array($form_state['values']['source'.$num_item], $use_type_identifiers)) ? 'source' : 'type';
      $id_source[$num_item] = (!in_array($form_state['values']['source'.$num_item], $use_type_identifiers)) ? $form_state['values']['source'.$num_item] : $form_state['values']['type'];
      $identifiers_names_source[$num_item] = explode(',', $form_values["identifiers_$type_source[$num_item]_$id_source[$num_item]"]);
        foreach ($identifiers_names_source[$num_item] as $identifier_key => $identifier_name)  {
        if (isset($field_values[$num_item][$type_source[$num_item]][$id_source[$num_item]][$identifier_key])) {
          $identifiers[$num_item][$identifier_name] = $field_values[$num_item][$type_source[$num_item]][$id_source[$num_item]][$identifier_key];
        }
      }
      if (empty($identifiers[$num_item])) {
        form_set_error('item'.$num_item, t('Please enter a valid value for the identifier.'));
      }
      elseif ($compound[$num_item][$type_source[$num_item]][$id_source[$num_item]] && count($identifiers_names_source[$num_item]) <> count($identifiers[$num_item])) {
        form_set_error('item'.$num_item, t('All the identifiers of a compound key should have valid values.'));
      }
    }
  }
  else {
    form_set_error('', t('Please enter valid values for the identifiers.'));
  }
  $form_state['values']['identifiers'] = $identifiers;
}

/**
* Implements hook_form_submit().
*/
function idskshub_content_mapping_form_submit($form, &$form_state) {
  $identifiers = $form_state['values']['identifiers'];
  $type_id = $form_state['values']['type'];
  $source1_id = $form_state['values']['source1'];
  $source2_id = $form_state['values']['source2'];
  IdsKsItem::mapItems($type_id, $source1_id, $source2_id, $identifiers);
}


function idskshub_combine_items_form($form, $form_state) {
  $form['#action'] = url('combine-items-confirm');
  $form['combine']['items'] = array(
    '#markup' =>  views_embed_view('merge_items', 'panel_pane_1'),
    '#weight' => 20,
    );  
  $form['actions']['next'] = array(
    '#type' => 'submit',
    '#value' => t('Combine selected items'),
//    '#prefix' => '<div style="float:right;">',
//    '#suffix' => '</div>', 
    '#weight' => 30,
    );
  return $form;
}

function idskshub_combine_items_confirm_form($form, $form_state) {
  $form_id = $form_state['input']['form_id'];
  if (($form_id == 'idskshub_combine_items_form') && (!(isset($form_state['input']['entity_ids']) && count($form_state['input']['entity_ids']) >= 2))) {
    drupal_set_message(t('Please select at least two items to combine.'), 'error'); 
    drupal_goto('combine-items');
  }
  else {
    if (isset($form_state['input']['entity_ids'])) {
      $items_ids = $form_state['input']['entity_ids'];
      $items = '<ul>';
      foreach ($items_ids as $item_id) {
        $item = new IdsKsItem();
        $item->load($item_id);
        if (isset($item->display_title)) {
          $item = $item->display_title;
        }
        else {
          $item = $item_id;
        }
        $items .= '<li><a href="item/' . $item_id . '">' . $item . '</a>';
      }
      $items .= '</ul>';
      $form['combine']['items'] = array(
        '#markup' =>  $items,
        );  
      $form['#action'] = url('combined-items', array('query' => $items_ids));
      $form['actions']['confirm'] = array(
        '#type' => 'submit',
        '#value' => t('Confirm'),
        );
    }
  }
  return $form;
}

/*
function idskshub_duplicates_form($form, $form_state) {
  $merging = idskshub_get('merging_items');
  if (!$merging) {
    $views_content = views_embed_view('duplicates', 'panel_pane_1');
    $form['duplicates']['items'] = array(
      '#markup' =>  $views_content,
      );  
    if (!preg_match('/view\-empty/', $views_content)) {
      $form['actions']['next'] = array(
        '#type' => 'submit',
        '#value' => t('Merge selected items'),
        '#prefix' => '<div style="float:right;">',
        '#suffix' => '</div>', 
        );
    }
  }
  else {
    $form['duplicates'] = array(
      '#markup' =>  t('Merging in process...'),
      );
  }
  return $form;
}

function idskshub_duplicates_form_validate($form, $form_state) {
  if(empty($form_state['input']['merge_items'])) {
    form_set_error('duplicates', t('Please select items to merge.'));  
  }
}

function idskshub_duplicates_form_submit($form, $form_state) {
  idskshub_process_duplicates($form_state['input']['merge_items']);
}
*/

/* ------------------------- Fields mappings ----------------------- */

/**
* Definition of the form.
*/
function idskshub_newmapping_form($form, $form_state) {
  $paths_destination = array_filter(array_unique(idskshub_flatten_array(IdsKsMapping::getFieldValues('path_destination'))));
  sort($paths_destination);
  array_unshift($paths_destination, '-- Add new --');
  $paths_destination[0] = '-- Add new --';
  $options_formats = array('' => 'All formats') + idskshub_get_formats_codes();
  $options_sources = array('' => 'All sources') + idskshub_get_sources_codes();
  $options_relationships = array(0 => 'Relationship type') + idskshub_get_relationship_ids();
  $options_actions = array('' => 'Do with field') + idskshub_mapping_actions();
  $form['mapping']['format_code'] = array(
    '#required' => '0',
    '#multiple' => '0',
    '#description' => t('The file format to assign the mapping to.'),
    '#weight' => '10',
    '#type' => 'select',
    '#options' => $options_formats,
    '#title' => t('Format'),
  );
  $form['mapping']['source_code'] = array(
    '#required' => '0',
    '#multiple' => '0',
    '#description' => t('The source to assign the mapping to.'),
    '#weight' => '10',
    '#type' => 'select',
    '#options' => $options_sources,
    '#title' => t('Source'),
  );
  $form['mapping']['origin'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Origin'), 
    '#weight' => 15, 
    '#collapsible' => FALSE, 
  );
  $form['mapping']['origin']['path_origin'] = array(
    '#title' => t('Path origin'),
    '#description' => t('Please enter the origin path with the tags that form it separated by "/".<br> 
      This represent a path in the source file and should be relative to the <b>root</b> element defined in the <a href="/formats">file format</a>.<br>
     (For instance: <i><b>results/list-item/archived</b></i>).<br>
      The origin path can also have the form: <i><b>*/tag_name</b></i>, where "*" is interpreted as "anything".
     '),
    '#weight' => '20',
    '#type' => 'textfield',
    '#maxlength'=> 255,
    '#size'=> 60,
    '#required' => '1',
  );
  $form['mapping']['origin']['attributes_origin'] = array(
    '#title' => t('Attributes'),
    '#description' => t('Please enter the attributes for the origin path in the form: "<i><b>attribute1=value1, attribute2=value2</b></i>".<br>'),
    '#weight' => '30',
    '#type' => 'textfield',
    '#maxlength'=> 255,
    '#size'=> 60,
    '#required' => '0',
  );
  $form['mapping']['destination'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Destination'), 
    '#weight' => 40, 
    '#collapsible' => FALSE, 
  );
  $form['mapping']['destination']['path_destination_select'] = array(
    '#required' => '0',
    '#multiple' => '0',
    '#description' => t('Select an existing hub field/path or add a new one below.'),
    '#weight' => '42',
    '#type' => 'select',
    '#options' => $paths_destination,
    '#title' => t('Mapped field'),
  );
  $form['mapping']['destination']['path_destination'] = array(
    //'#title' => t('Mapped field'),
    '#description' => t('Please enter the destination path with the tags that form it separated by "/". (For instance: <i><b>archived</b></i>).<br>
      This field is required if no alternative action is selected.<br>
      When possible, use <a href="@hub_fields_url" target="_new">existing hub fields</a>.
     ',
     array('@hub_fields_url' => '/hub-fields')),
    '#weight' => '45',
    '#type' => 'textfield',
    '#maxlength'=> 255,
    '#size'=> 50,
    '#required' => '0',
    '#states' => array(
      'enabled' => array(
        ':input[name="path_destination_select"]' => array('value' => 0),
      ),
    ),
  );
  $form['mapping']['destination']['action'] = array(
    '#required' => '0',
    '#multiple' => '0',
    '#description' => t('Action to execute (Optional).'),
    '#weight' => '50',
    '#type' => 'select',
    '#options' => $options_actions,
    '#title' => t('Action'),
  );
  $form['mapping']['destination']['relationship'] = array(
    '#default_value' =>  '0',
    '#required' => '0',
    '#multiple' => '0',
    '#description' => t('If the field should be processed as an independent item (eg: Region), indicate the relationship to the parent item (eg: Region of focus) (Optional).'),
    '#weight' => '60',
    '#type' => 'select',
    '#options' => $options_relationships,
    '#title' => t('Relationship'),
    '#states' => array(
      'enabled' => array(
        ':input[name="action"]' => array('value' => 'RELATED_ITEM'),
      ),
    ),
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  return $form;
}

/**
* Implements hook_form_validate().
*/
function idskshub_newmapping_form_validate($form, &$form_state) {
  if (idskshub_check_path($form_state['values']['path_origin'], TRUE)) {
    $form_state['values']['path_origin'] = trim($form_state['values']['path_origin'], '/');
    if ($form_state['values']['path_destination_select']) {
      $paths_destination_select = array_filter(array_unique(idskshub_flatten_array(IdsKsMapping::getFieldValues('path_destination'))));
      sort($paths_destination_select);
      $form_state['values']['path_destination'] = $paths_destination_select[$form_state['values']['path_destination_select'] - 1];
    }
    elseif (idskshub_check_path($form_state['values']['path_destination'])) {
      $form_state['values']['path_destination'] = trim($form_state['values']['path_destination'], '/');
    }
    elseif (!in_array($form_state['values']['action'], array_keys(idskshub_mapping_actions()))) {
      form_set_error('path_destination', t('The destination path is not valid. Please enter a valid path or select an action to execute.'));
    }
  }
  else {
      form_set_error('path_origin', t('The origin path is not valid.'));
  }
}

/**
* Implements hook_form_submit().
*/
function idskshub_newmapping_form_submit($form, &$form_state) {
  $format_code = $form_state['values']['format_code'];
  $source_code = $form_state['values']['source_code'];
  $path_origin = $form_state['values']['path_origin'];
  $attributes_origin = $form_state['values']['attributes_origin'];
  $path_destination = $form_state['values']['path_destination'];
  $relationship = $form_state['values']['relationship'];
  $action = $form_state['values']['action'];
  $mapping = IdsKsMapping::createMappingFromStrings($path_origin, $path_destination, $format_code, $source_code, $attributes_origin, $relationship, $action);
  if ($mapping->getId()) {
    drupal_set_message(t('Mapping proccessed correctly. <a href="/ids_ks_mapping/ids_ks_mapping/@id">View mapping</a>.', array('@id' => $mapping->getId())));
  }
}

/* ------------------------ Import mappings ----------------------- */

/**
* Definition of the form.
*/
function idskshub_mappings_form($form, $form_state) {
  $options_formats = array('' => 'All formats') + idskshub_get_formats_codes();
  $options_sources = array('' => 'All sources') + idskshub_get_sources_codes();
  $form['mappings_file']['format_code'] = array(
    '#required' => '0',
    '#multiple' => '0',
    '#description' => t('File format to assign the mappings to. It is overriden with the format codes included the file).'),
    '#weight' => '10',
    '#type' => 'select',
    '#options' => $options_formats,
    '#title' => t('Format'),
  );
  $form['mappings_file']['source_code'] = array(
    '#required' => '0',
    '#multiple' => '0',
    '#description' => t('Source to assign the mappings to. It is overriden with the source codes included the file).'),
    '#weight' => '10',
    '#type' => 'select',
    '#options' => $options_sources,
    '#title' => t('Source'),
  );
  $form['mappings_file']['replace'] = array(
    '#title' => t('Add / replace existing mappings'),
    '#type' => 'radios',
    '#default_value' => 'add',
    '#weight' => '20',
    '#options' => array(
      'add' => t('Add to / update existing mappings'),
      'replace' => t('Remove existing mappings and create new'),
    ),
  );
  $form['mappings_file']['file'] = array(
    '#title' => t('Select file'),
    '#description' => 
      t('Upload a comma-separated CSV file with one mapping per row.') . '<br>' .  
      t('Valid formats: <b>[format_code],[source_code],path_origin,["atr1=val1,at2=val2"],[path_destination],[action],[relationship]</b>. The format and source codes can be left empty for mappings to be applied to all formats/sources.') . '<br>' .
      t('The attributes can be left empty. If there are several pairs of attributes/values, they should be enclosed in double quotes: <b><i>"atr1=val1, atr2=val2"</i></b>') . '<br>' .
      t('Example: <ul><li><i><b>idsxml,,results/list-item/category_region_array/region/list-item,,,RELATED_ITEM,Region of focus</b></i></ul>') . '<br>' .
      t('Please note that this action cannot be undone. <b>Consider <a href="@download_mappings" target="_new">backing up the existing mappings</a> before proceeding</b>.', array('@download_mappings' => 'mappings-export')),
    '#type' => 'managed_file',
    '#upload_validators' => array('file_validate_extensions' => array('xml rss csv txt')),
    '#size' => '30',
    '#weight' => '30',
    '#required' => '1',
  );
  $form['actions']['next'] = array(
    '#type' => 'submit',
    '#value' => t('Create / update mappings'),
  );
  return $form;
}

/**
* Implements hook_form_validate().
*/
function idskshub_mappings_form_validate($form, &$form_state) {
}

/**
* Implements hook_form_submit().
*/
function idskshub_mappings_form_submit($form, &$form_state) {
  $fid = $form_state['values']['file'];
  $format_code = $form_state['values']['format_code'];
  $source_code = $form_state['values']['source_code'];
  $replace = $form_state['values']['replace'];
  IdsKsMapping::createMappingsFile($fid, $format_code, $source_code, $replace);
}

// Display links to the elements that are referring a given element of type $type with id $id.
function idskshub_display_usedby($id, $type) {
  $num_showed = 50;
  $show_message = '';
  if ($id) {
    switch ($type) {
      case 'source':
        $feeds_ids = IdsKsFeed::getParentsIds(array('source' => $id));
        $mappings_ids = IdsKsMapping::getParentsIds(array('source' => $id));
        $literals_ids = IdsKsLiteral::getParentsIds(array('source' => $id));
        $track_changes_ids = IdsKsTrackChanges::getParentsIds(array('source' => $id));
        break;
      case 'contributor':
        $sources_ids = IdsKsSource::getParentsIds(array('contributor' => $id));
        break;
      case 'type':
        $items_ids = IdsKsItem::getParentsIds(array('type' => $id));
        $feeds_ids = IdsKsFeed::getParentsIds(array('default_type' => $id));
        break;
      case 'format':
        $mappings_ids = IdsKsMapping::getParentsIds(array('format' => $id));
        $feeds_ids = IdsKsFeed::getParentsIds(array('default_format' => $id));
        break;
      case 'item':
        $items_ids = IdsKsItem::getParentsIds(array('subitems' => $id));
        break;
    }
    if (!empty($feeds_ids) || !empty($mappings_ids)  || !empty($literals_ids) || !empty($track_changes_ids) || !empty($items_ids) || !empty($sources_ids)) {
      echo t('The @type is being rererred to by the following elements:', array('@type' => $type))  . '<br>';
      echo '<ul>';
      if (!empty($sources_ids)) {
        $entities = entity_load('ids_ks_source', $sources_ids);
        array_walk($sources_ids, function(&$input) use($entities) {$input='<a href="/ids_ks_source/ids_ks_source/'.$input.'">'.entity_label('ids_ks_source', $entities[$input]).'</a>';});
        echo '<li> <b>Sources:</b><ul> ' . implode('<li> ', $sources_ids) . '</ul><br>';
      }
      if (!empty($feeds_ids)) {
        $entities = entity_load('ids_ks_feed', $feeds_ids);
        array_walk($feeds_ids, function(&$input) use($entities) {$input='<a href="/ids_ks_feed/ids_ks_feed/'.$input.'">'.entity_label('ids_ks_feed', $entities[$input]).'</a>';});
        echo '<li> <b>Feeds:</b><ul> ' . implode('<li> ', $feeds_ids) . '</ul><br>';
      }
      if (!empty($mappings_ids)) {
        $entities = entity_load('ids_ks_mapping', $mappings_ids);
        array_walk($mappings_ids, function(&$input) use($entities) {$input='<a href="/ids_ks_mapping/ids_ks_mapping/'.$input.'">'.entity_label('ids_ks_mapping', $entities[$input]).'</a>';});
        echo '<li> <b>Mappings:</b><ul> ' . implode('<li> ', $mappings_ids) . '</ul><br>';
      }
      if (!empty($items_ids)) {
        $count = count($items_ids);
        if ($count > $num_showed) {
          $items_ids = array_slice($items_ids, 0, $num_showed);
          $show_message = " (Displayed first $num_showed of " . $count . ")";
        }
        $entities = entity_load('ids_ks_item', $items_ids);
        array_walk($items_ids, function(&$input) use($entities) {$input='<a href="/item/'.$input.'">'.entity_label('ids_ks_item', $entities[$input]).'</a>';});
        echo '<li> <b>Items:</b>'.$show_message.'<ul> ' . implode('<li> ', $items_ids) . '</ul><br>';
      }
      if (!empty($literals_ids)) {
        $count = count($literals_ids);
        if ($count > $num_showed) {
          $literals_ids = array_slice($literals_ids, 0, $num_showed);
          $show_message = " (Displayed first $num_showed of " . $count . ")";
        }
        $entities = entity_load('ids_ks_literal', $literals_ids);
        array_walk($literals_ids, function(&$input) use($entities) {$input='<a href="/literals/'.$input.'">'.entity_label('ids_ks_literal', $entities[$input]).'</a>';});
        echo '<li> <b>Literals:</b>'.$show_message.'<ul> ' . implode('<li> ', $literals_ids) . '</ul><br>';
      }
      if (!empty($track_changes_ids)) {
        $count = count($track_changes_ids);
        if ($count > $num_showed) {
          $track_changes_ids = array_slice($track_changes_ids, 0, $num_showed);
          $show_message = " (Displayed first $num_showed of " . $count . ")";
        }
        $entities = entity_load('ids_ks_track_changes', $track_changes_ids);
        array_walk($track_changes_ids, function(&$input) use($entities) {$input='<a href="/ids_ks_track_changes/ids_ks_track_changes/'.$input.'">'.entity_label('ids_ks_track_changes', $entities[$input]).'</a>';});
        echo '<li> <b>Track changes:</b>'.$show_message.'<ul> ' . implode('<li>', $track_changes_ids) . '</ul><br>';
      }
      echo '</ul>';
    }
    else {
      echo t('The @type is currently not being rererred to by other elements.', array('@type' => $type))  . '<br>';
    }
  }
}

/* These functions to print HTML tables of -for instance- the sources and languages available are not being used currently */
function idskshub_display_table($values, $header=array(), $style='', $class='') {
  echo '<table ';
  if ($style) { echo 'style="'.$style.'" ';}
  if ($class) { echo 'class="'.$class.'" ';}
  echo '>';
  idskshub_display_table_row($header);
  idskshub_display_table_row($values);
  echo '</table>';
}

function idskshub_display_table_row($values) {
  foreach ($values as $key => $value) {
    echo "<tr>";
    echo "<td>$key</td>";
    echo "<td>$value</td>";
    echo "</tr>\n";
  }
}

function idskshub_display_sources_table() {
  idskshub_display_table(idskshub_get_sources_codes(), array('<b>Code</b>' => '<b>Name</b>'));
}

function idskshub_display_languages_table() {
  idskshub_display_table(idskshub_supported_languages(), array('<b>ISO code</b>' => '<b>Language</b>'));
}
