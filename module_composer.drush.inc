<?php

/**
 * Created by IntelliJ IDEA.
 * User: jan.schumann
 * Date: 19.01.15
 * Time: 21:54
 */

function module_composer_drush_command() {
  $items = array();

  $items['mc-compose'] = array(
    'callback' => 'module_composer_compose',
    'description' => "Download, Enable, Disable, Uninstall modules by configuration.",
    'drupal dependencies' => array('dic'),
	'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL
  );

  $items['mc-dump-config'] = array(
	'callback' => 'module_composer_dump_config',
	'description' => "Dump current module configuration."
  );

  return $items;
}

/**
 * Init the container and collect installed projects
 */
function _module_composer_init() {
  // check requirements
  if (!function_exists('drupal_dic') || !drupal_dic() || !drupal_dic()->hasParameter('kernel.config_dir')) {
    drush_set_error('DRUSH_MODULE_COMPOSER_INIT_ERRPR', "Drupal dic initialisation failed. Is dic enabled and properly configured?");
  }

  if (!class_exists('Drupal\ModuleComposer\Kernel')) {
    drush_set_error('DRUSH_MODULE_COMPOSER_INIT_ERRPR', "Autoloading is not properly configured. Please require janschumann/module_composer to your composer.json.");
  }

  // collect installed projects
  _module_composer_dic()->get('mc.project_collector')->collect();
}

/**
 * Implements @see drush_COMMAND_init()
 */
function drush_mc_compose_init() {
  _module_composer_init();
}

/**
 * Command callback sp-make
 */
function module_composer_compose() {
  // invoke necessary drush commands
  foreach (_module_composer_command_list() as $command => $rows) {
    $command = "pm-$command";
    if ($command == "pm-download") {
      drush_set_option("cache", true);
    }

    foreach ($rows as $row) {
      _module_composer_dic()->get('mc.drush_bridge')->invoke($command, array($row));
    }

    // cache option is only valid for download so remove it after modules are downloaded
    drush_unset_option("cache");
  }

  // set variables
  if (_module_composer_dic()->hasParameter('mc.variables')) {
    drush_set_option('exact', true);
    foreach (_module_composer_dic()->getParameter('mc.variables') as $name => $value) {
      _module_composer_dic()->get('mc.drush_bridge')->invoke('variable-set', array($name, (string) $value));
    }
    drush_unset_option('exact');
  }
}

/**
 * @return array
 */
function _module_composer_command_list() {
  // process profiler
  $projects = _module_composer_dic()->get('mc.configured_projects');
  $list = _module_composer_dic()->get('mc.composer')->getToDo($projects);
  _module_composer_prepare_commands($list);

  return $list;
}

/**
 * @param array $command_list
 */
function _module_composer_prepare_commands(&$command_list) {
  foreach ($command_list as $command => $rows) {
    if (empty($rows)) {
      // remove empty commands
      unset($command_list[$command]);
      continue;
    }
    // prepare the arguments
    foreach ($rows as $i => $row) {
      $command_list[$command][$i] = implode(',', array_unique($row));
    }
  }
}

/**
 * Implements @see drush_COMMAND_init()
 */
function drush_mc_dump_config_init() {
  _module_composer_init();
}

/**
 * Command callback sp-dump-config
 */
function module_composer_dump_config() {
  print _module_composer_dic()->get('mc.config_dumper')->dump();
}

/**
 * @return Symfony\Component\DependencyInjection\Container
 */
function _module_composer_dic() {
  static $kernel;

  if (!isset($kernel)) {
    $kernel = new \Drupal\ModuleComposer\Kernel(drupal_dic()->getParameter('kernel.environment'), drupal_dic()->getParameter('kernel.debug'), drupal_dic()->getParameter('kernel.root_dir'), drupal_dic()->getParameter('kernel.config_dir'));
    $kernel->boot();
  }

  return $kernel->getContainer();
}

