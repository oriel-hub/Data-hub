<?php
// $Id$

/**
 * @file
 * IDS KS Related Item class.
 *
 */

class IdsKsRelatedItem extends IdsKsObject {
  public $source;
  public $relationship;
  public $referred_item;
  public $path;

  public function createInverse($item) {
    if (($relationship = $this->get('relationship')) && ($inverse_rel_code = $relationship->get('inverse')) && ($referred_item = $this->get('referred_item')) && ($source = $this->get('source'))) {
      if ($inv_rels_ids = IdsKsRelationship::getIds(array('code' => $inverse_rel_code, 'item_type' => $item->get('type')->getId()))) {
        $inverse_relationship = new IdsKsRelationship;
        $inverse_relationship->load($inv_rels_ids[0]);
        if ($inverse_relationship->isSaved()) {
          $rel_item = new IdsKsRelatedItem;
          $rel_item->set('referred_item', $item);
          $rel_item->set('relationship', $inverse_relationship);
          $rel_item->set('source', $source);
          if (!$referred_item->relatedItemExists($rel_item)) {
            $rel_item->save();
            if ($rel_item->getId()) {
              $referred_item->add('related_items', $rel_item);
              $referred_item->save();
            }
          }
        }
      }
    }

  }

} // class IdsKsRelatedItem

