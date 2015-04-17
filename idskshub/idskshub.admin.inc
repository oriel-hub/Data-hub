<?php
// $Id;

/**
 * @file
 *   IDS KS Hub Module global settings form.
 *
 * Display and allows to change global settings for the module.
 *
 */

/**
 * Creates the form settings.
 */
function idsapi_settings_form() {
  $form = array();
  return system_settings_form($form);
}

/**
 * Validates the form settings input.
 */
function idsapi_settings_form_validate($form, &$form_state) {

}

