<?php
// $Id$

/**
 * @file
 * Additional IDS KS classes without methods are kept here.
 *
 */

require_once('idskshub.object.inc');
require_once('idskshub.idsksattribute.inc');
require_once('idskshub.idsksitem.inc');
require_once('idskshub.idsksliteral.inc');
require_once('idskshub.idsksmapping.inc');
require_once('idskshub.idskstype.inc');
require_once('idskshub.idsksversionsliteral.inc');
require_once('idskshub.idsksrelateditem.inc');

// See if this is useful in practice. This information could go in the source.
class IdsKsContributor extends IdsKsObject {
  public $name;
  public $acronym;
  public $website;
  public $vocabulary; // Not using this at this stage.
  public $description;
  public $email;
  public $logo;
  public $partner;
  public $priority;
  public $location;
  public $type;
  public $organisation_type_term;
  public $contact_name;
  public $contact_role;
  public $contact_email;
} // class IdsKsContributor

class IdsKsFeed extends IdsKsObject {
  public $name;
  public $url;
  public $source;
  public $default_type;
  public $default_format;
  public $xslt_file;
  public $periodicity;
  public $weekday;
  public $crontab; //Currently not being used.
} // class IdsKsFeed

class IdsKsExternalMappings extends IdsKsObject {
  public $source_origin;
  public $source_destination;
  public $mappings_url;
  public $external_property;
  public $relationship;
  public $relationship_source;
} // class IdsKsExternalMappings

class IdsKsExternalUris extends IdsKsObject {
  public $source;
  public $type;
  public $uris_structure;

  public function getValuesFromUri($uri) {
    $values = array();
    preg_match_all('/%(?!%).+?%/', $this->get('uris_structure'), $matches);
    $keys = $matches[0];
    $regexp = '@' . preg_replace('/%(?!%).+?%/', '(.+)', $this->get('uris_structure')) . '@';
    preg_match($regexp, $uri, $vals);
    if ($vals) {
      foreach ($keys as $pos => $key) {
        $key = trim($key, '%');
        $values[$key] = $vals[$pos+1];
      }
    }
    return $values;
  }
} // class IdsKsExternalUris

class IdsKsFormat extends IdsKsObject {
  public $code;
  public $name;
  public $item_paths;
  public $root_path;
  public $type_path;
  public $xslt_file;
} // class IdsKsFormat

class IdsKsRelationship extends IdsKsObject {
  public $type;
  public $item_type;
  public $code;
  public $inverse;
  public $description;
} // class IdsKsRelationship

class IdsKsSource extends IdsKsObject {
//  public $type;
  public $name;
  public $code;
  public $contributor;
  public $import_url;
  public $default_language;
  public $identifiers; // For example, in the case of Eldis documents, <root>:'object_id'.
  public $compound_key; // If identifiers should be connected by AND.
  public $is_semantic; // Indicates if the source corresponds to an external semantic augmentation service (eg: Open Calais).
  public $description;
  public $logo;
  public $website;
  public $theme;
  public $keywords;
  public $licence_description;
  public $licence_url;
  public $taxonomies_in_use;
  public $contact_name;
  public $contact_role;
  public $contact_email;
  public $is_external;
  public $endpoint_url;
  public $lifespan;
  public $external_properties;
  public $editorial_policy;
} // class IdsKsSource

class IdsKsTrackChanges extends IdsKsObject {
  public $source;
  public $timestamp;
  public $raw_data;
  public $hash;

  public static function hashExists($hash) {
    return self::getIds(array('hash' => $hash));
  }
  public static function newHash($data) {
    return hash('md5', $data);
  }
} // class IdsKsTrackChanges




/* These classes are not being used - DELETE

class IdsKsTag extends IdsKsObject {
  public $name;
  //public $vocabulary;
} // class IdsKsTag

class IdsKsTranslation extends IdsKsObject {
  public $lang_orig;
  public $lang_trans;
  public $term_orig;
  public $term_trans;
}  // class IdsKsTranslation

class IdsKsVocabulary extends IdsKsObject {
  public $prefix;
  public $url_namespace;

  public static function getVocabulary($prefix='', $url_namespace='') {
    $voc = new IdsKsVocabulary();
    if ($prefix) {
      $voc->prefix = $prefix;
    }
    else {
      $voc->prefix = IDSKS_DEFAULT_VOCABULARY_PREFIX;
    }
    if ($url_namespace) {
      $voc->url_namespace = $url_namespace;
    }
    $voc->load();
    if (!$voc->getId()) {
      $voc->save();
    }
    return $voc;
  }
} // class IdsKsVocabulary
*/


