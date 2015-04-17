<?php

$cambiar_temas = array(
'Natural resources and the environment.Climate change' => array('Climate change','Eldis,Observaction,SocioEconomic Research Portal for the Philippines,Open Docs,3ie Documents'),
'Health.Sexual & reproductive health' => array('Sexual & reproductive health','HEART'),
'Health systems.Equity' => array('Equity','HEART'),
'HIV.HIV and the health system.International policy and aid financing' => array('International policy and aid financing','HEART'),
'Norway.water' => array('Water','Open Docs'),
'Natural resources and the environment.Agriculture' => array('Agriculture','Open Docs'),
'Climate change.Technology' => array('Technology','Open Docs'),
'Health and well-being.HIV/AIDS' => array('HIV/AIDS','Open Docs'),
'Norway.Aid' => array('Aid','Open Docs'),
'Nutrition.Nutrition sensitive development.Agriculture' => array('Agriculture','Open Docs'),
'Nutrition.Nutrition sensitive development.Education' => array('Education','Open Docs'),
'Trade Policy.Environment' => array('Environment','Open Docs'),
'Aid effectiveness.Millennium Development Goals (MDGs)' => array('Millennium Development Goals (MDGs)','Open Docs'),
'Infrastructure and services.Rural development' => array('Rural development','Open Docs'),
'Poverty.Social protection' => array('Social protection','Open Docs,3ie Documents'),
'Education.finance' => array('Finance','Open Docs,3ie Documents'),
'Livelihoods.Gender' => array('Gender','Open Docs,3ie Documents'),
'Trade Policy.Environment.environment and natural resources' => array('Environment and natural resources','SocioEconomic Research Portal for the Philippines'),
'Rising powers in international development.Health and social policy.Health' => array('Health','SocioEconomic Research Portal for the Philippines,HEART,Open Docs'),
'HIV.HIV and the health system.Governance' => array('Governance','SocioEconomic Research Portal for the Philippines,Open Docs'),
);

$sources = array();
$type_theme = new IdsKsType;
$type_theme->load(63);
foreach ($cambiar_temas as $titulo_viejo => $cambiar) {
  list($titulo_nuevo, $fuentes) = $cambiar;
  $nombres_fuentes = explode(',', $fuentes);
  $cambiar_sources = array();
  foreach ($nombres_fuentes as $nombre_fuente) {
    if (!isset($sources[$nombre_fuente])) {
      $s = new IdsKsSource;
      $s->name = $nombre_fuente;
      $s->load();
      $sources[$nombre_fuente] = $s;
      $cambiar_sources[] = $s;
    }
    else {
      $cambiar_sources[] = $sources[$nombre_fuente];
    }
  }
  $theme_viejo = new IdsKsItem;
  $theme_viejo->display_title = $titulo_viejo;
  $theme_viejo->load();
  if ($theme_viejo->getId()) {
    $theme_nuevo = new IdsKsItem;
    $theme_nuevo->display_title = $titulo_nuevo;
    $theme_nuevo->load();
    if (!$theme_nuevo->getId()) {
      echo "Creando nuevo theme para $titulo_nuevo.\n";
      $nuevo = TRUE;
      $theme_nuevo->type = $type_theme;
      $theme_nuevo->save();
    } 
      else {
      echo "Theme $titulo_nuevo existe. Modificandolo...\n";
      $nuevo = FALSE;
    }
    $versions_literals_nuevo = $theme_nuevo->getVersionsLiteralPath('title');
    foreach ($cambiar_sources as $source) {
      if ($theme_viejo->hasContentFromSource($source->getId())) {
        echo 'Borrando literales de ' . $source->code . " en item $titulo_viejo - " . $theme_viejo->getId() . "\n";
        $theme_viejo->deleteLiteralsSource($source);
      }
      echo 'Agregando literal (titulo) de ' . $source->code . " a item $titulo_nuevo - " . $theme_nuevo->getId() . "\n";
      $versions_literals_nuevo->addLiteralXML($titulo_nuevo, $source, 'title');
    }
    $theme_viejo->saveItem(TRUE, FALSE);
    $theme_nuevo->saveItem(TRUE, FALSE);
    foreach ($cambiar_sources as $source) {
      $related_items_ids = IdsKsRelatedItem::getParentsIds(array('source' => $source->getId(), 'referred_item' => $theme_viejo->getId()));
      echo "Cambiando $titulo_viejo - " . $theme_viejo->getId() . " por $titulo_nuevo - " . $theme_nuevo->getId() . ' en rel. items:' . "\n";
      print_r($related_items_ids);
      $related_items_entities = entity_load('ids_ks_related_item', $related_items_ids);
      foreach ($related_items_entities as $entity_related_item) {
        $related_item_wrapper = entity_metadata_wrapper('ids_ks_related_item', $entity_related_item);
        $related_item_wrapper->field_related_item_referred_item = $theme_nuevo->getId();
        $related_item_wrapper->save();    
      }
    }
  }
  else {
    echo "Theme $titulo_viejo no encontrado.\n";
  }
}




