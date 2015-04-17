<?php

$source = new IdsKsSource;
$source->load(15);

$relationship = new IdsKsRelationship;
$relationship->load(4);

$item = new IdsKsItem;
$item->load(21430);

$related_item = new IdsKsRelatedItem;
$related_item->source = $source;
$related_item->relationship = $relationship;
$related_item->referred_item = $item;
$related_item->path = 'category_theme_array';
$related_item->save();

//print_r($related_item);

$i = new IdsKsItem;
$i->load(9440);

$i->related_items[] = $related_item;
$i->save();


