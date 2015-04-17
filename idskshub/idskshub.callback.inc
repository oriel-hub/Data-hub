<?php
// $Id$

/**
 * @file
 *  IDS KS Hub Callback functions.
 *
 * Include here the implementation of callback functions used to pre-process imported values and referred to in field mappings.
 *
 * Required parameters of the callback functions:
 * xml_value
 *    SimpleXMLElement corresponding to the value being processed.
 * source
 *    Optional. IdsKsSource object corresponding to the source being imported.
 * xml_path
 *    Optional. The current path as a string.
 * attributes
 *    Optional. Array of IdsKsAttribute objects containing the attribute/value pairs inherited by the current element.
 *
 * Return value:
 * xml_new_value
 *    SimpleXMLElement containing the new value to be processed instead of xml_value or FALSE on failure.
 *
 */

// This function can be used to retrieve a remote single value when $xml_value contains a URL.
function okhub_retrieve_single_value(SimpleXMLElement $xml_value, IdsKsSource $source, $xml_path, array $attributes, $wrapper='single') {
  $url = html_entity_decode((string) $xml_value);
  $dom = new SmartDOMDocument;
  $dom->loadXML(file_get_contents($url));
  if ($wrapper) {
    $new_root = $dom->createElement($wrapper);
    $new_root->appendChild($dom->documentElement);
    $dom->appendChild($new_root);
  }
  $xml_new_value = simplexml_import_dom($dom);
  return $xml_new_value;
}

// This function can be used to retrieve external values when $xml_value contains multiple URLs.
function okhub_retrieve_multiple_values(SimpleXMLElement $xml_value, IdsKsSource $source, $xml_path, array $attributes) {
  $dom = new DOMDocument;
  $new_root = $dom->createElement('list');
  $dom->appendChild($new_root);
  foreach ($xml_value as $xml_tag => $xml_single_value) { 
    $sxe = okhub_retrieve_single_value($xml_single_value, $source, $xml_path, $attributes, '');
    $dom_sxe = dom_import_simplexml($sxe);
    $dom_sxe = $dom->importNode($dom_sxe, true);
    $new_root->appendChild($dom_sxe);
  }
  $xml_new_value = simplexml_import_dom($dom);
  return $xml_new_value;
}






