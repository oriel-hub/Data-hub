<?php
// $Id$

/**
 * @file
 *  IDS KS Hub settings.
 */

// TODO: Admin interface to manage these settings.

define('IDSKS_OKHUB_SOURCE_CODE', 'okhub');
define('IDSKS_HUB_SOURCE_NAME', 'OKHub');
define('IDSKS_HUB_SOURCE_CODE', 'hub'); // Source for internal use.
define('IDSKS_FROM_EMAIL_ADDRESS', 'IDS Open Knowledge Hub <okhub@ids.ac.uk>');
define('IDSKS_FEEDS_UPDATE_MODE', 'replace'); // Possible values: 'incremental' (partial updates) / 'replace' (total updates).
define('IDSKS_FEEDS_UPDATE_FIELDS', 'replace'); // Only used if IDSKS_FEEDS_UPDATE_MODE == 'incremental'. Possible values: 'replace' (replaces existing) / 'allow_multiple' (adds new values to existing without duplicates).
define('IDSKS_FEEDS_IMPORT_UNCHANGED', FALSE); // Indicates if unchanged items should be skipped (default) or re-imported.
define('IDSKS_FEEDS_SEMANTIC_ANNOTATION', FALSE); // Call to semantic annotation services when importing feeds. TODO: Alternative: run from cron.
define('IDSKS_FEEDS_INDEX', FALSE); // Immediately index imported feeds (default: wait until indexing process launched from cron).
define('IDSKS_NUM_ELEMENTS_INTERACTIVE', 500);
define('IDSKS_GENERIC_TYPE_NAME', 'Structure');
define('IDSKS_DEFAULT_TYPE_ITEMS', 'Document');
define('IDSKS_DEFAULT_FORMAT', 'rssxml');
define('IDSKS_DEFAULT_LANGUAGE', 'en');
define('IDSKS_UNDEFINED_LANGUAGE', 'un');
define('IDSKS_UNDEFINED_LANGUAGE_INDEX', 'zx');
define('IDSKS_NOT_APPLICABLE_LANGUAGE', 'zz');
define('IDSKS_MULTIPLE_LANGUAGE', 'zx');
define('IDSKS_SOURCES_FIELD', 'sources');
define('IDSKS_LANGUAGE_ATTRIBUTE', 'lang');
define('IDSKS_LANGUAGES_FIELD', 'metadata_languages');
define('IDSKS_LANGUAGES_XML_FIELD', 'hub_metadata_languages');
define('IDSKS_DEFAULT_MAPPED_FIELD', 'name'); // Used when no destination field is defined in the mapping and a new related item has to be created.
define('IDSKS_DEFAULT_MAPPING_REL_CATEGORY', 'exactMatch');
define('IDSKS_DEFAULT_MAPPING_REL_ASSET', 'sameAs');
define('IDSKS_DEFAULT_PARENT_REL_CATEGORY', 'broader');
define('IDSKS_DEFAULT_CHILD_REL_CATEGORY', 'narrower');
define('IDSKS_EXCLUDE_EXTERNAL_FROM_INDEX', FALSE); // Prevent items from external sources from being included in the Solr index.
if ($GLOBALS ['base_url'] == 'http://data.test.okhub.org') { // Development server
  define('IDSKS_OKHUB_URL', 'http://data.test.okhub.org');
  define('IDSKS_SOLR_SERVER', 'http://data.test.okhub.org:8983');
  define('IDSKS_SOLR_CORE', 'oriel-dev');
  define('IDSKS_SOLR_INDEX', 'okhub_solr_index_dev');
  define('IDSKS_URL_API', 'http://api.test.okhub.org/v1/hub');
  define('IDSKS_URL_EXPLORER', 'http://explorer.test.okhub.org');
  define('IDSKS_FILES_URL', 'http://data.test.okhub.org/sites/data.test.okhub.org/files');
}
elseif ($GLOBALS ['base_url'] == 'http://data.okhub.org') { // Live server
  define('IDSKS_OKHUB_URL', 'http://data.okhub.org');
  define('IDSKS_SOLR_SERVER', 'http://data.okhub.org:8983');
  define('IDSKS_SOLR_CORE', 'okhub-live');
  define('IDSKS_SOLR_INDEX', 'ids_ks_item_sources_index');
  define('IDSKS_URL_API', 'http://api.okhub.org/v1/hub');
  define('IDSKS_URL_EXPLORER', 'http://explorer.okhub.org');
  define('IDSKS_FILES_URL', 'http://data.okhub.org/sites/data.okhub.org/files');
}

