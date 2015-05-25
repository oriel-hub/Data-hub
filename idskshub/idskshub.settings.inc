<?php
// $Id$

/**
 * @file
 *  IDS KS Hub settings.
 */

define('IDSKS_FROM_EMAIL_ADDRESS', 'IDS Open Knowledge Hub <okhub@ids.ac.uk>');
define('IDSKS_UNDEFINED_SOURCE_ID', 4);
define('IDSKS_NUM_ELEMENTS_INTERACTIVE', 500);
define('IDSKS_HUB_SOURCE_CODE', 'hub');
define('IDSKS_OKHUB_SOURCE_CODE', 'okhub');
define('IDSKS_HUB_SOURCE_NAME', 'OKHub');
define('IDSKS_GENERIC_TYPE_NAME', 'Structure');
define('IDSKS_DEFAULT_TYPE_ITEMS', 'Document');
define('IDSKS_DEFAULT_FORMAT', 'rssxml');
define('IDSKS_DEFAULT_LANGUAGE', 'en');
define('IDSKS_UNDEFINED_LANGUAGE', 'un');
define('IDSKS_UNDEFINED_LANGUAGE_INDEX', 'zx');
define('IDSKS_NOT_APPLICABLE_LANGUAGE', 'zz');
define('IDSKS_MULTIPLE_LANGUAGE', 'zx');
define('IDSKS_SOURCES_FIELD', 'sources'); // allow to change this through the admin interface.
define('IDSKS_SOLR_INDEX', 'ids_ks_item_sources_index');
//define('IDSKS_SOLR_INDEX', 'ok_hub_solr_index_temp');
define('IDSKS_LANGUAGE_ATTRIBUTE', 'lang');
define('IDSKS_LANGUAGES_FIELD', 'metadata_languages');
define('IDSKS_LANGUAGES_XML_FIELD', 'hub_metadata_languages');
define('IDSKS_SOLR_SERVER', 'data.okhub.org:8983');
define('IDSKS_SOLR_CORE', 'okhub-live');
//define('IDSKS_SOLR_CORE', 'okhub-temp');
define('IDSKS_OKHUB_URL', 'http://data.okhub.org');
define('IDSKS_DEFAULT_MAPPED_FIELD', 'name'); // Used when no destination field is defined in the mapping and a new related item has to be created.
define('IDSKS_URL_LANGUAGES', 'http://api.okhub.org/v1/hub/get/language');

// TODO. Manage the options below through the admin interface.

function idskshub_supported_languages() {
  $languages = array(
    'en' => t('English'),
    'es' => t('Spanish'),
    'fr' => t('French'),
    'pt' => t('Portuguese'),
    'de' => t('German'),
    'it' => t('Italian'),
    'nl' => t('Dutch'),
  );
  return $languages;
}

function idskshub_mapping_actions() {
  $actions = array(
    'PREPROCESS' => 'PREPROCESS: Pre-process the content of the (XML) element by calling to a function)',
    'RELATED_ITEM' => 'RELATED_ITEM: Associate to a related item (select relationship type below)',
    'IGNORE' => 'IGNORE: Ignore a structured (XML) element',
    'SKIP' => 'SKIP: Skip one level of a structured (XML) element and continue processing its sub-elements',
    'AS_STRING' => 'AS_STRING: Import a structured (XML) element as a string',
    'SPLIT' => 'SPLIT: Import a comma-separated value as a set of different elements',
  );
  return $actions;
}
