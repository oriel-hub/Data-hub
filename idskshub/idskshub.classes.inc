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

// See if this is useful in practice. This information could go in the source.
class IdsKsContributor extends IdsKsObject {
  public $name;
  public $acronym;
  public $public_url;
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
  public $periodicity;
  public $weekday;
  public $crontab; //Currently not being used.
} // class IdsKsFeed

class IdsKsFormat extends IdsKsObject {
  public $code;
  public $name;
  public $item_paths;
  public $root_path;
  public $type_path;
} // class IdsKsFormat

class IdsKsRelatedItem extends IdsKsObject {
  public $source;
  public $relationship;
  public $referred_item;
  public $path;
} // class IdsKsRelatedItem

class IdsKsRelationship extends IdsKsObject {
  public $type;
  public $item_type;
  public $code;
} // class IdsKsRelationship

class IdsKsSource extends IdsKsObject {
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


