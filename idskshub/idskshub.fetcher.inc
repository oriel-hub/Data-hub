<?php
// $Id$

/**
 * @file
 *   IdsKsFetcher class.
 *
 */

class IdsKsFetcher {

  public static function fetchFile($fid, $url, $source_id, $feed_id, $format_code, $type_name, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $index_imported, $force_import) {
    $args = array();
    $error = FALSE;
    $file = FALSE;
    $feed = NULL;
    if ($feed_id) {
      $feed = new IdsKsFeed();
      $feed->load($feed_id);
    }
    if (!$source_id) {
      if ($feed->get('source')) {
        $source = $feed->get('source');
      }
      else {
        $source = new IdsKsSource();
        $source->set('code', IDSKS_HUB_SOURCE_CODE);
        $source->load();
      }
    }
    else {
      $source = new IdsKsSource();
      $source->load($source_id);
    }
    if ($fid) {
      $file = file_load($fid);
    }
    elseif ($url) {
      $file = system_retrieve_file($url, NULL, TRUE, FILE_EXISTS_RENAME, array('timeout' => 150));
    }
    elseif ($feed->get('url')) { // The file was not set, look for the feeds's import URL.
      $url = $feed->get('url');
      $file = system_retrieve_file($url, NULL, TRUE, FILE_EXISTS_RENAME, array('timeout' => 150));
    }
    if (!empty($file)) {
      IdsKsParser::parseFile($file, $source, $feed, $format_code, $type_name, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $index_imported, $force_import);
    }
    else {
      drupal_set_message(t('The file could not be retrieved.'), 'error');
    }
  }

}

