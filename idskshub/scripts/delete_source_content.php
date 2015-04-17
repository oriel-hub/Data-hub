<?php

$source_id = 21;
$lits = IdsKsLiteral::getParentsIds(array('source' => $source_id));
foreach ($lits as $lit) {
  IdsKsLiteral::deleteLiteral($lit);
  echo "deleting literal $lit\n";
}

