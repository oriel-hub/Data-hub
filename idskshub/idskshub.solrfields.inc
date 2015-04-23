<?php 
// $Id$

/**
 * @file
 *   IdsKsHub definition of fields to be indexed.
 */

/* ----------------------------- Hub's fields definition ---------------------------- */

/* Note: IDSKS_DEFAULT_LANGUAGE is defined in idskshub.settings.inc */

/**
*  Fields naming convention:
* - {field_name}_{source}_{language_code}
* Special cases:
* - zx - language unknown / combined fields
* - zz - language not applicable
*/


// Generate fields of structured metadata_languages.
function idskshub_get_metadata_languages_fields($lang_code) {
  include_once DRUPAL_ROOT . '/includes/iso.inc';
  $languages = _locale_get_predefined_list();
  $value = '';
  if (isset($languages[$lang_code])) {
    $value .= '<object_id>' . $lang_code . '</object_id>';
    $value .= '<object_type>language</object_type>';
    $value .= '<object_name>' . $languages[$lang_code][0] . '</object_name>';
    if (isset($languages[$lang_code][1])) {
      $value .= '<original_name>' . $languages[$lang_code][1] . '</original_name>';
    }
    $value .= '<metadata_url>' . IDSKS_URL_LANGUAGES . '/' . $lang_code . '</metadata_url>';
  }
  return $value;
}

/** 
/*  This function defines the parameters to index values in related items' fields (such as categories).
/*  It is used, for instance, to generate category arrays to add to Eldis and BRIDGE's items .
/*  These fields can be then used to generate additional values (by the functions defined below). 
/*  The keys in 'related_item_fields' should match mapped paths in the items.
/*  (Alternative paths can be used joined by '|'. The first matching path will be used to retrieve the field's value).
*/
function idskshub_index_related_items_fields() {
  $index_related_items_as_xml = array( // Indexed by source code and relationship code.
    'eldis' => array(
      'theme' => array(
        'indexed_field_name' => 'category_theme_array', // Generated field name to index. The source and language codes will be appended automatically.
        'external_wrappers' => array('themeList'),
        'related_item_wrappers' => array('theme'),
        'related_item_fields' => array(
                                  'archived' => 'archived',
                                  'level' => 'level',
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
      'region' => array(
        'indexed_field_name' => 'category_region_array',
        'external_wrappers' => array('regionList'),
        'related_item_wrappers' => array('Region'),
        'related_item_fields' => array(
                                  'archived' => 'archived',
                                  'deleted' => 'deleted',
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
      'subject' => array(
        'indexed_field_name' => 'category_subject_array',
        'external_wrappers' => array('subjectList'),
        'related_item_wrappers' => array('subject'),
        'related_item_fields' => array(
                                  'archived' => 'archived',
                                  'level' => 'level',
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
      'publisher' => array(
        'indexed_field_name' => 'publisher_array',
        'external_wrappers' => array('publisherList'),
        'related_item_wrappers' => array('Publisher'),
        'related_item_fields' => array(
                                  'country' => 'Country',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                  'role' => 'role',
                                  'metadata_url' => 'metadata_url',
                                ),
      ),
      'country_focus' => array(
        'indexed_field_name' => 'country_focus_array',
        'external_wrappers' => array('countryList'),
        'related_item_wrappers' => array('Country'),
        'related_item_fields' => array(
                                  'alternative_name' => 'alternative_name', 
                                  'iso_two_letter_code' => 'iso_two_letter_code', 
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
    ),
    'bridge' => array(
      'theme' => array(
        'indexed_field_name' => 'category_theme_array',
        'external_wrappers' => array('themeList'),
        'related_item_wrappers' => array('theme'),
        'related_item_fields' => array(
                                  'archived' => 'archived',
                                  'level' => 'level',
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
      'region' => array(
        'indexed_field_name' => 'category_region_array',
        'external_wrappers' => array('regionList'),
        'related_item_wrappers' => array('Region'),
        'related_item_fields' => array(
                                  'archived' => 'archived',
                                  'deleted' => 'deleted',
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
      'subject' => array(
        'indexed_field_name' => 'category_subject_array',
        'external_wrappers' => array('subjectList'),
        'related_item_wrappers' => array('subject'),
        'related_item_fields' => array(
                                  'archived' => 'archived',
                                  'level' => 'level',
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
      'country_focus' => array(
        'indexed_field_name' => 'country_focus_array',
        'external_wrappers' => array('countryList'),
        'related_item_wrappers' => array('Country'),
        'related_item_fields' => array(
                                  'alternative_name' => 'alternative_name', 
                                  'iso_two_letter_code' => 'iso_two_letter_code', 
                                  'metadata_url' => 'metadata_url',
                                  'object_type' => 'object_type',
                                  'object_id' => 'object_id',
                                  'name|title|object_name' => 'object_name',
                                ),
      ),
    ),
  );
  return $index_related_items_as_xml;
}

// TODO: Implement admin interface to manage these settings.
/**
 * The arrays returned by this function allow to indicate additional fields
 * to be generated by the hub - to be used as generic values for 
 * a (possible multi-valued) field with content from multiple sources and/or 
 * in multiple languages, as well as fields holding values of the field
 * to be used as search filter and for sorting search results.
 * 
 * For each field, the name(s) of the new fields should be included in one
 * or more of the arrays associated to the keys 'generic', 'search' and 'sort'.(*)
 *
 * For each new field is also has to be indicated how to generate their values.
 * This latter is done by including, associated to the definition of the
 * new field, an array with key 'construction'. 
 * Possible values for 'construction' are:
 * - For 'generic' fields: 'single'|'combine' (default: 'single').
 * - For 'search' fields: 'single'|'combine' default: 'combine').
 * - For 'sort' fields: 'single'.
 *  
 * The value 'single' means that only one value will be assigned, while
 * 'combine' means that multiple values can be assigned.
 * (Additional definitions about how to select/filter these values can be
 *  set in idskshub_get_solr_hub_fields_values_definitions()).
 *
 * There is  also the possibility of including the key 'copy' to indicate
 * that the value of a *previously defined field* should be copied.
 * (In this case, if a 'construction' key is present, it will be ignored and
 * the copied field definition will be used).
 *
 * (*) Note that currently, the only difference between these arrays is which
 * method will be used  by default to construct the values of the new fields in special
 * cases  (such as when there is an error if the definition of the 'construct'
 * method), but in the future, this information will also be used to generate the
 * corresponding schema entry for the new fields.
 */

define('IDSKS_SOLR_LANGUAGES_FIELD', IDSKS_LANGUAGES_FIELD . '_hub_zz');

function idskshub_get_solr_hub_fields_definitions() {
  $hub_types = idskshub_get_types_names();
  $fields_settings = array(
    /* Hub's generic fields - In general, single-valued. Can be multi-valued (combined) */
    'generic' => array( 
        // For 'single' fields, if not defined in idskshub_get_solr_hub_fields_values_definitions() below, defaults to filter: 'language' => IDSKS_DEFAULT_LANGUAGE (= 'en').
        'title' => array('title_hub_zx' => array('construction' => 'single')),
        'description' => array('description_hub_zx' => array('construction' => 'single')), 
        'date_created' => array('date_created_hub_zz' => array('construction' => 'single')),
        'date_updated' => array('date_updated_hub_zz' => array('construction' => 'single')),
        'country_focus_ids' => array('country_code_hub_zz' => array('construction' => 'combine')),
        'item_type' => array(
                        'hub_item_type' => array('construction' => 'single'), // value to display to the users.
                        'object_type_hub_zz' => array('construction' => 'single'), // value being used internally by the API.
                      ),
        'item_id' => array(
                      'hub_item_id' => array('construction' => 'single'), // value to display to the users.
                      'object_id_hub_zz' => array('construction' => 'single'), // value being used internally by the API.
                    ),

      ),
    /* Search filters - In general, multi-valued (combined). Can be single-valued */
    /* All searchable fields are also copied into Solr's 'content' field. */
    'search' => array(
        'item_id' => array('content' => array('copy' => 'hub_item_id')),
        'title' => array(
          'title_search_hub_zx' => array('construction' => 'combine'),
          'title_search_hub_en' => array('construction' => 'combine'),
          'title_autocomplete_hub_zx' => array('copy' => 'title_hub_zx'),
          'content' => array('copy' => 'title_search_hub_zx'),
        ),
        'description' => array('content' => array('construction' => 'combine')),
        'author' => array(
          'author_search_hub_zz' => array('construction' => 'combine'),
          'author_facet_hub_zz' => array('copy' => 'author_search_hub_zz'),
          'content' => array('copy' => 'author_search_hub_zz'),
        ),
        'keyword' => array(
          'keyword_search_hub_zx' => array('construction' => 'combine'),
          'keyword_facet_hub_zx' => array('copy' => 'keyword_search_hub_zx'),
          'content' => array('copy' => 'keyword_search_hub_zx'),
        ),
        'country_focus' => array(
          'country_focus_search_hub_zx' => array('construction' => 'combine'), 
          'country_focus_facet_hub_zx' => array('copy' => 'country_focus_search_hub_zx'),
          'content' => array('copy' => 'country_focus_search_hub_zx'),
        ),
        'country_code' => array(
          'country_code_search_hub_zz' => array('copy' => 'country_code_hub_zz'),
          'country_code_facet_zz' => array('copy' => 'country_code_search_hub_zz'),
        ),
        'document_type' => array(
          'document_type_search_hub_zx' => array('construction' => 'combine'), 
          'document_type_facet_hub_zx' => array('copy' => 'document_type_search_hub_zx'),
          'content' => array('copy' => 'document_type_search_hub_zx'),
        ),
        'metadata_languages' => array(
          'metadata_languages_hub_zz' => array('construction' => 'combine'),
          'metadata_languages_search_hub_zx' => array('copy' => 'metadata_languages_hub_zz'),
        ),        
        'category_theme_array' => array(
          'category_theme_search_hub_zx' => array('construction' => 'combine'), 
          'category_theme_facet_hub_zx' => array('copy' => 'category_theme_search_hub_zx'),
          'content' => array('copy' => 'category_theme_search_hub_zx'),
        ),
        'object_id' => array('content' => array('construction' => 'combine')),
/*
        'object_id' => array(
          'object_id_search_hub_zz' => array('construction' => 'combine'), 
          'object_id_facet_hub_zz' => array('copy' => 'object_id_search_hub_zz'),
          'content' => array('copy' => 'object_id_search_hub_zz'),
        ),
        'object_type' => array(
          'object_type_search_hub_zx' => array('construction' => 'combine'), 
          'object_type_facet_hub_zx' => array('copy' => 'object_type_search_hub_zx'),
          'content' => array('copy' => 'object_type_search_hub_zx'),
        ),
*/
      ),
    /* Sort fields - Can only be single-valued */
    'sort' => array(
        'title' => array('title_sort_hub_zx' => array('copy' => 'title_hub_zx')),
        'date_created' => array('date_created_sort_hub_zz' => array('copy' => 'date_created_hub_zz')),
        'date_updated' => array('date_updated_sort_hub_zz' => array('copy' => 'date_updated_hub_zz')),
        'category_path' => array('category_path_sort_hub_zz' => array('construction' => 'single')), 
        'name' => array('name_sort_hub_en' => array('construction' => 'single')), 
        'object_id' => array('object_id_sort_hub_zz' => array('construction' => 'single')), 
        'acronym' => array('acronym_sort_hub_en' => array('construction' => 'single')), 
        'category_id' => array('category_id_sort_hub_zz' => array('construction' => 'single')), 
        'asset_id' => array('asset_id_sort_hub_zz' => array('construction' => 'single')), 
        'publication_date' => array('publication_date_sort_hub_zz' => array('construction' => 'single')), 
        'publication_year' => array('publication_year_sort_hub_zz' => array('construction' => 'single')), 
      ),
    );
  return $fields_settings;
}

// TODO: Implement admin interface to manage these settings.
/**
 * The arrays returned by this function allow to indicate how the fields created
 * by the hub (single or combined) are generated.
 *
 * These settings are *optional* to allow overring the default methods for
 * constructing the values (for both 'single' or 'combine' fields).
 *
 * Possible filters (both for 'single' and 'combined' values):
 * - language: two-letter code|'all' (default is IDSKS_DEFAULT_LANGUAGE).
 *   It is ignored if the field is not translatable.
 * - source: code name (by default, values from all sources are considered).
 * - regexp: keeps only those values that match the regular expression.
 *
 * Additionally filter:
 * - priority: 'language'|'source' allows to indicate, when there are not values
 * available for the source and language, whether the language or the source values
 * should be prioritised (it's only considered if the source is set and the field is translatable).
 * The default is 'language'.
 *
 * Selection of single values: 
 * Additionally, for 'single' values, the 'select' parameter allows to override the way
 * in which the value is chosen.
 * Options are: 'highest'|'lowest|'latest'|'first'. If not set, the default value is 'latest'.
 * highest/lowest - refer to the ascending order of the values.
 * latest/first - refer to the date in which the value was updated in the hub.
 */
function idskshub_get_solr_hub_fields_values_definitions() {
  $fields_construction = array(
    'single' => array(
        //'title_hub_en' => array('source' => 'eldis'),
        //'description_hub_en' => array('source' => 'eldis'),
        'title_hub_zx' => array('language' => IDSKS_DEFAULT_LANGUAGE, 'select' => 'latest'),
        'description_hub_zx' => array('language' => IDSKS_DEFAULT_LANGUAGE, 'select' => 'latest'),
        'date_created_hub_zz' => array('select' => 'lowest'),
        'date_updated_hub_zz' => array('select' => 'highest'),
        'category_path_sort_hub_zz' => array('source' => 'eldis'),
        'name_sort_hub_en' => array('source' => 'eldis'),
        'object_id_sort_hub_zz' => array('source' => 'eldis'),
        'acronym_sort_hub_en' => array('source' => 'eldis'),
        'category_id_sort_hub_zz' => array('source' => 'eldis'),
        'asset_id_sort_hub_zz' => array('source' => 'eldis'),
        'publication_date_sort_hub_zz' => array('source' => 'eldis'),
      ),
    'combine' => array (
        'title_search_hub_zx' => array('language' => 'all'),
        'title_search_hub_en' => array('language' => 'en'),
        'keyword_search_hub_zx' => array('language' => 'all'),
        'content' => array('language' => 'all'),
        'country_code_hub_zz' => array('regexp' => '/^\s*\w{2}\s*$/'),
      ),
  );
  return $fields_construction;       
}

/**
 * Fields that are required by the API. If they don't exist, their value is copied from the field
 * that is indicated.
 * The new field can be used to generate additional (generic, search or sort) fields.
 */

function idskshub_get_solr_hub_required_fields() {
  $fields_settings = array(
    'title' => 'name',
    );
  return $fields_settings;
}

/* ---------------------------- End of hub's fields definition --------------------------- */

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

// Fields only used for faceted searches in Drupal (required by Search API). - *** Not being used currently ***.
// TODO: Is there a Facet API function to retrieve these fields from the database?
function idskshub_solr_facets_fields() {
  $facets = array(
              'field_item_type:field_type_name' => 'ss_search_api_type_hub_zx',
              'field_item_track_changes:field_track_changes_timestamp' => 'dm_search_api_date_updated_hub_zz',
              'field_item_track_changes:field_track_changes_source:field_source_name' => 'sm_search_api_source_hub_zz',
            );
  return $facets;
}


