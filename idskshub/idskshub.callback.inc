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
 *    IdsKsSource object corresponding to the source being imported.
 * xml_path
 *    The current path as a string. It can be changed in order to use the processed values / path in other mappings.
 * attributes
 *    Array of IdsKsAttribute objects containing the attribute/value pairs inherited by the current element.
 */

// This function can be used to retrieve a remote single value when $xml_value contains a URL.
function okhub_retrieve_single_value(SimpleXMLElement &$xml_value, IdsKsSource $source, &$xml_path, array $attributes) {
  $newdoc = new DOMDocument;
  $newdoc->loadXML('<root/>');
  $urldoc = new SmartDOMDocument;
  $url = html_entity_decode((string) $xml_value);
  if ($urldoc->load($url)) {
    $node = $urldoc->documentElement;
    $node = $newdoc->importNode($node, true);
    $newdoc->documentElement->appendChild($node);
  }
  $xml_attributes = $xml_value->attributes();
  $xml_value = simplexml_import_dom($newdoc);
  foreach ($xml_attributes as $attr_name => $attr_value) {
    $xml_value->addAttribute($attr_name, $attr_value);
  }
}

// This function can be used to retrieve external values when $xml_value contains multiple URLs.
function okhub_retrieve_multiple_values(SimpleXMLElement &$xml_value, IdsKsSource $source, &$xml_path, array $attributes) {
  $newdoc = new DOMDocument;
  $newdoc->loadXML('<root/>');
  $urldoc = new SmartDOMDocument;
  foreach ($xml_value as $xml_tag => $xml_single_value) { 
    $url = html_entity_decode((string) $xml_single_value);
    if ($urldoc->load($url)) {
      $node = $urldoc->documentElement;
      $node = $newdoc->importNode($node, true);
      $newdoc->documentElement->appendChild($node);
    }
  }
  $xml_attributes = $xml_value->attributes();
  $xml_value = simplexml_import_dom($newdoc);
  foreach ($xml_attributes as $attr_name => $attr_value) {
    $xml_value->addAttribute($attr_name, $attr_value);
  }
}

function okhub_split_values(SimpleXMLElement &$xml_value, IdsKsSource $source, &$xml_path, array $attributes, $sep = '[,\;]') {
  $dom = new DOMDocument;
  $new_root = $dom->createElement('root');
  $dom->appendChild($new_root);
  $arr_path = explode('/', $xml_path); // We use the same element to add the splitted values.
  $tag = array_pop($arr_path);
  $xml_path = implode('/', $arr_path);
  foreach (preg_split('/\s*'.$sep.'\s*/', (string)$xml_value) as $value) {
    $child = $dom->createElement($tag, $value);
    $new_root->appendChild($child);
  }
  $xml_attributes = $xml_value->attributes();
  $xml_value = simplexml_import_dom($dom);
  foreach ($xml_attributes as $attr_name => $attr_value) {
    $xml_value->addAttribute($attr_name, $attr_value);
  }
}


function okhub_split_slash_values(SimpleXMLElement &$xml_value, IdsKsSource $source, &$xml_path, array $attributes) {
  okhub_split_values($xml_value, $source, $xml_path, $attributes, '\/');
}

// This function checks that the element contains some alphanumeric character(s) in it or sets it to NULL so the value is skipped.
function okhub_check_alphanumeric(SimpleXMLElement &$xml_value, IdsKsSource $source, &$xml_path, array $attributes) {
  if (!preg_match('/[\w\d]/', (string) $xml_value)) {
    $xml_value = NULL;
  }
  else {
    $arr_path = explode('/', $xml_path); // We remove the last part of the path so it's not added twice.
    array_pop($arr_path);
    $xml_path = implode('/', $arr_path);
    $xml_attributes = $xml_value->attributes();
    $xml_value = simplexml_load_string("<?xml version='1.0'?><root>".$xml_value->asXML()."</root>");
    foreach ($xml_attributes as $attr_name => $attr_value) {
      $xml_value->addAttribute($attr_name, $attr_value);
    }
  }
}

// This function checks that the element contains a valid URL or sets it to NULL so the value is skipped.
function okhub_check_url(SimpleXMLElement &$xml_value, IdsKsSource $source, &$xml_path, array $attributes) {
  if (!filter_var((string) $xml_value, FILTER_VALIDATE_URL)) {
    $xml_value = NULL;
  }
  else {
    $arr_path = explode('/', $xml_path); // We remove the last part of the path so it's not added twice.
    array_pop($arr_path);
    $xml_path = implode('/', $arr_path);
    $xml_attributes = $xml_value->attributes();
    $xml_value = simplexml_load_string("<?xml version='1.0'?><root>".$xml_value->asXML()."</root>");
    foreach ($xml_attributes as $attr_name => $attr_value) {
      $xml_value->addAttribute($attr_name, $attr_value);
    }
  }
}

// This function flattens an XML element so it's imported as string.
function okhub_flatten_xml(SimpleXMLElement &$xml_value, IdsKsSource $source, &$xml_path, array $attributes) {
  $arr_path = explode('/', $xml_path);
  $xml_element = array_pop($arr_path);
  $xml_path = implode('/', $arr_path);
  $xml_attributes = $xml_value->attributes();
  foreach ($xml_value as $xml_tag => $xml_item) {
    $value .= htmlspecialchars($xml_item->asXML());
  }
  $xml_value = simplexml_load_string("<?xml version='1.0'?><root><$xml_element>".$value."</$xml_element></root>");
  foreach ($xml_attributes as $attr_name => $attr_value) {
    $xml_value->addAttribute($attr_name, $attr_value);
  }
}



