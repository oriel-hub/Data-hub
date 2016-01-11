<?php
// $Id$

/**
 * @file
 *   IdsKsParser class.
 *
 */

require_once('smartdomdocument/SmartDOMDocument.class.php');

class IdsKsParser {
  
  public static function parseFile($file, $source, $feed, $format_code, $type_name, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $index_imported, $force_import) {
    if ($format_code) {
      $format = new IdsKsFormat();
      $format->set('code', $format_code);
      $format->load();
    }
    elseif ($feed && $feed->get('default_format')) {
      $format = $feed->get('default_format');
    }
    else {
      drupal_set_message(t('The input format could not be retrieved. Please make sure that the feed has a default input format defined. RSS-XML assumed.'), 'warning');
      $format = new IdsKsFormat();
      $format->set('code', IDSKS_DEFAULT_FORMAT);
      $format->load();
    }
    if ($format->getId()) {
      $xslt_file = NULL;
      if ($feed && $feed->get('xslt_file')) {
        $xslt_file = $feed->get('xslt_file');
      }
      elseif ($format->get('xslt_file')) {
        $xslt_file = $format->get('xslt_file');
      }
      $items = array();
      $file_path = drupal_realpath($file->uri);
      list($item_path, $items, $total_elements, $root_file_attributes) = IdsKsParser::parseXML($file_path, $format, $xslt_file);
      if (!empty($items)) {
        IdsKsProcessor::batchProcess($items, $item_path, $source, $feed, $format, $type_name, $total_elements, $mode, $semantic, $existing_items, $update_mode, $allow_multiple, $index_imported, $force_import, $root_file_attributes);
      }
      else {
        drupal_set_message(t('No items identified in the source file.'), 'warning');
        if (isset($feed)) {
          drupal_set_message(t('Please check the "Default format" in the <a href="ids_ks_feed/ids_ks_feed/@feed_id">feed\'s settings</a>. Also verify that the "Root path" and "Item paths" settings in the <a href="ids_ks_format/ids_ks_format/@format_id">format</a> are correct.', array('@feed_id'=>$feed->getId(), '@format_id'=>$format->getId())));
        }
        else {
          drupal_set_message(t('Please verify that the "Root path" and "Item paths" settings in the <a href="ids_ks_format/ids_ks_format/@format_id">format</a> are correct.', array('@source_id'=>$source->getId(),'@format_id'=>$format->getId())));
        }
      }
    }
    else {
      drupal_set_message(t('The input format is not supported.'), 'error');
    }
  }
  
  public static function parseXML($file_path, $format, $xslt_file) {
    $items = array();
    $item_paths = array();
    $item_path = '';
    $root_file_attributes = array();
    $total_elements = 0;
    $file_contents = file_get_contents($file_path);
    $file_contents = trim($file_contents);
    $file_contents = mb_convert_encoding($file_contents, 'UTF-8');
    $file_contents = self::changeNamespaces($file_contents);
    // decode entities before cleaning the HTML up.
    // This might generate problems with the XML parser with badly formed HTML (making it skip the rest of the file).
    // TODO: Use the tidy lib to clean included HTML code. To install Tidy PHP common should be updated in the system.
    /*
    $file_contents = html_entity_decode($file_contents);
    $file_contents = self::removeHTML($file_contents);
    $file_contents = self::changeAllowedHTML($file_contents);
    */
    if (!empty($file_contents)) {
      $xml = new DOMDocument;
      $xml->recover = TRUE;
      if ($xml_loaded_ok = $xml->loadXML($file_contents, LIBXML_NOCDATA|LIBXML_ERR_NONE)) {
        if ($xslt_file && isset($xslt_file[3])) {
          $xslt_file_uri = $xslt_file[3];
          if ($file_xslt_path = drupal_realpath($xslt_file_uri)) {
            $xsl = new DOMDocument;
            $xsl->substituteEntities = true;
            if ($xsl->load($file_xslt_path)) {
              $proc = new XSLTProcessor;
              $proc->importStyleSheet($xsl); // attach the xsl rules
              $file_contents = $proc->transformToXML($xml);
              $xml_loaded_ok = $xml->loadXML($file_contents, LIBXML_NOCDATA|LIBXML_ERR_NONE);
            }
            else {
              drupal_set_message(t('The XSLT file @file could not be loaded. Please check the file format.', array('@file'=>$file_xslt_path)), 'warning');
            }
          }
          else {
            drupal_set_message(t('The XSLT tranformations could not be applied. The file @file was not loaded.', array('@file'=>$$xslt_file_uri)), 'warning');
          }
        }
        $dom = new SmartDOMDocument; // Quick fix to continue loading the file as HTML (which converts tags to lowercase, etc). For backward compatibility.
        $dom->recover = TRUE;
        if ($xml_loaded_ok && $dom->loadHTML($xml->saveHTML())) {
          if ($xml_obj = simplexml_import_dom($dom)) {
            $item_path = '';
            $root_path = '';
            $xml_root = '';
            if ($format->get('root_path')) {
              $root_path = $format->get('root_path');
            }
            if ($format->get('item_paths')) {
              $item_paths = $format->get('item_paths');
            }
            if ($root_path) {
              $root_string = '//' . idskshub_change_namespace($root_path);
              $root = $xml_obj->xpath($root_string);
              if (!empty($root)) {
                $xml_root = end($root);
                $root_file_attributes = (array) $xml_root->attributes();
              }
            }
            if (!empty($item_paths)) {
              foreach ($item_paths as $path) {
                if ($items = idskshub_get_xml_values($xml_obj, $path, $root_path)) {            
                  $item_path = $path;
                  break;
                }
              }
              $total_elements = count($xml_obj->xpath('//*')); // Total XML elements, to know the volume (items, subitems, literals) to import and switch to background.
            }
          }
          else {
            drupal_set_message(t('Error while parsing the XML file @file.', array('@file'=>$file_path)), 'error');
          }
        }
        else {
          drupal_set_message(t('The XML file @file could not be loaded as HTML. Please check the file format.', array('@file'=>$file_path)), 'warning');
        }
      }
      else {
        drupal_set_message(t('The XML file @file could not be loaded. Please check the file format.', array('@file'=>$file_path)), 'warning');
      }
    }
    return array($item_path, $items, $total_elements, $root_file_attributes);
  }

  public static function changeNamespaces($str) { // We change how the namespaces are indicated so they are not ignored by the HTML parser.
    $str = preg_replace('/<([\-\w]+):([\-\w]+)/', '<\1--\2', $str );    
    $str = preg_replace('/<\/([\-\w]+):([\-\w]+)>/', '</\1--\2>', $str );    
    $str = preg_replace('/\s+([\-\w]+):([\-\w]+)\s*=/', ' \1--\2=', $str );   
    return $str;
  }

  public static function removeHTML($str) { // We remove some html tags that we don't want to include in the database.
    $str = preg_replace("/<\/?(abbr|applet|aside|audio|b|basefont|bdo|big|blockquote|button|canvas|caption|center|cite|code|col|colgroup|command|datalist|dd|del|dfn|dir|div|dl|dt|em|embed|fieldset|figcaption|figure|font|footer|form|frame|frameset|h\d|head|header|hgroup|hr|html|i|iframe|img|input|ins|keygen|kbd|label|legend|link|map|mark|menu|meta|meter|nav|noframes|noscript|ol|optgroup|option|output|pre|progress|q|rp|rt|ruby|s|samp|script|section|select|small|source|span|strike|strong|style|sub|sup|table|tbody|td|textarea|tfoot|th|thead|time|tr|tt|u|var|video|wbr|xml|xmp)((\s+\w+(\s*=\s*(?:\".*?\"|'.*?'|[^'\">\s]+))?)+\s*|\s*)\/?>/i" , "", $str);
    $str = preg_replace("/<!--.*-->/" , "", $str);
    $str = preg_replace('/\s*(style|class|id)\s*=\s*["\'][a-zA-Z0-9:;\.\s\(\)\-\,]+["\']/Ui', '', $str);
    //$str = preg_replace('/\s\s+/', ' ', $str );
    //$str = preg_replace('/[\.]+/', '.', $str ); //<!--[if gte mso 9]>
    return $str;
  }

  public static function changeAllowedHTML($str) {
    $allowed_html = array('a', 'p', 'ul', 'li', 'br');
    $tags = '(' . implode('|', $allowed_html) . ')';
    $str = preg_replace('/<'.$tags.'(\s+.*|\/|)>/Ui', '[\1\2]', $str);
    $str = preg_replace('/<\/'.$tags.'>/Ui', '[/\1]', $str);
    return $str;
  }

  public static function restoreAllowedHTML($str) {
    $allowed_html = array('a', 'p', 'ul', 'li', 'br');
    $tags = '(' . implode('|', $allowed_html) . ')';
    $str = preg_replace('/\['.$tags.'(\s+.*|\/|)\]/Ui', '<\1\2>', $str);
    $str = preg_replace('/\[\/'.$tags.'\]/Ui', '</\1>', $str);
    return $str;
  }


} // IdsKsParser class. 
  

