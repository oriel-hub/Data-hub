<?php

$type = new IdsKsType;
$type->load(63);

$i = new IdsKsItem;
$i->type = $type;
$themes = $i->getObjectsByProperties();

foreach ($themes as $theme) {
  $sources = array();
  foreach ($theme->getSourcesFromValues() as $source) {
    $sources[] = $source->code;
  }
  echo $theme->display_title . ';' . implode('', $sources) . "\n";
}



