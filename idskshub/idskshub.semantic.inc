<?php 
// $Id$

/**
 * @file
 *   IdsKsHub definition of external semantic services
 */

// Include required functions to use the external services.
require_once('opencalais/opencalais.php');
define('IDSKS_OPENCALAIS_KEY', 'rdffdz49td9vjufwrb72ngj5');

/**
 * This function returns an array with the list of the (source code names) of available semantic services.
 * If a source with this code name does not exist, it will be created.
 * Please note that *only lower case letter without accents* can be used in source names.
 */
// TODO: Manage this through the admin interface!
function idskshub_semantic_services_definitions() {
  $semantic = array(
      'opencalais' => 'Open Calais',
    );
  return $semantic;
}

/**
 * This function calls to the external semantic augmentation service with the item's data (XML) to be analysed.
 * The return value should be an array indexed by the keywords returned by the semantic augmentation service.
 * For instance:
 * $semantic_entities = array (
          ['Continent'] => array ('Africa'),
          ['Country'] => array ('Chad', 'Nigeria'),
      );
   $service_code is the source code name of the service to be called.
   $data contains the data to be send to the external service to be processed. Its format depends on the service.
  */
function idskshub_semantic_services_call($service_code, $data) {
  switch ($service_code) {
    case 'opencalais':
      $oc = new OpenCalais(IDSKS_OPENCALAIS_KEY);
      $semantic_entities = $oc->getEntities($data);
      break;
  }
  return($semantic_entities);
}
