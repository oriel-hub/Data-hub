<?php

$tits_od = array(
"“God made the soil, but we made it fertile”: Gender, knowledge and practice in the formation and use of African Dark Earths in Liberia and Sierra Leone",
"Innovation & Intellectual Property: Collaborative Dynamics in Africa",
"The political ecology of soybean farming systems in Mato Grosso, Brazil: a cross-scale analysis of farming styles in Querência-MT",
);

foreach ($tits_od as $t) {
  echo "processing: $t\n";
  $i = new IdsKsItem;
  $i->display_title = $t;
  $items = $i->getObjectsByProperties();
  if (empty($items)) {
    echo "no items found\n";
  }
  foreach ($items as $item) {
    if( $item->isEmptyItem()) {
      echo 'deleting ' . $item->getId() . "\n";
      $item->delete(TRUE, FALSE);
    }
    else {
      echo 'keeping ' . $item->getId() . "\n";
    }
  }
}

echo "done\n";
