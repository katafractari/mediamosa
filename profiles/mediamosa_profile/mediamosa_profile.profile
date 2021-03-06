<?php
/**
 * @file
 * MediaMosa installation profile.
 */

// Include our helper class as autoloader is not up yet.
require_once 'mediamosa_profile.class.inc';
require_once 'sites/all/modules/mediamosa/core/storage/mediamosa_storage.class.inc';

/**
 * Implements hook_install_tasks().
 */
function mediamosa_profile_install_tasks() {

  // Add our css.
  drupal_add_css('profiles/mediamosa_profile/mediamosa_profile.css');

  // Set our title.
  drupal_set_title(mediamosa_profile::get_title());

  // Setup the tasks.
  return array(
    'mediamosa_profile_storage_location_form' => array(
      'display_name' => st('Storage location'),
      'type' => 'form',
      'run' => variable_get('mediamosa_current_mount_point', '') ? INSTALL_TASK_SKIP : INSTALL_TASK_RUN_IF_NOT_COMPLETED,
    ),
    'mediamosa_profile_metadata_support_form' => array(
      'display_name' => st('Metadata support'),
      'type' => 'form',
    ),
    'mediamosa_profile_apache_settings_form' => array(
      'display_name' => st('Apache settings'),
      'type' => 'form',
    ),
    'mediamosa_profile_cron_settings_form' => array(
      'display_name' => st('Cron settings'),
      'type' => 'form',
    ),
  );
}

/**
 * Implements hook_install_tasks_alter().
 */
function mediamosa_profile_install_tasks_alter(&$tasks, $install_state) {

  // Our requirements step.
  $mediamosa_profile_php_settings_form = array(
    'display_name' => st('MediaMosa requirements'),
    'type' => 'form',
  );

  // We need to rebuild tasks in the same order and put our
  // 'mediamosa_profile_php_settings_form' between it.
  $tasks_rebuild = array();
  foreach ($tasks as $name => $task) {
    // Copy task.
    $tasks_rebuild[$name] = $task;

    // If we reach certain point, then insert our task.
    if ($name == 'install_bootstrap_full') {
      $tasks_rebuild['mediamosa_profile_php_settings_form'] = $mediamosa_profile_php_settings_form;
    }
  }

  // Copy rebuild.
  $tasks = $tasks_rebuild;
}

/**
 * Implements hook_form().
 *
 * Select the metadata set(s).
 */
function mediamosa_profile_metadata_support_form() {
  $form = array();

  $options = array(
    'dublin_core' => st('Dublin Core'),
    'qualified_dublin_core' => st('Qualified Dublin Core'),
  );

  $form['description'] = array(
    '#markup' => '<p><b>' . st('Select the types of Metadata you want to support in your MediaMosa installation. Any of these metadata libraries can be enabled or disabled later by enabling or disabling the metadata module of its type.') . '</b></p>',
  );

  $form['metadata_support'] = array(
    '#type' => 'checkboxes',
    '#title' => st('Select the metadata libraries you want to use.'),
    '#description' => st('For more information about Dublin Core see !link_dc. For more information about Qualified Dublin Core !link_qdc.', array(
      '!link_dc' => l(t('dublincore.org'), 'http://dublincore.org', array('absolute' => TRUE, 'attributes' => array('target' => '_blank'))),
      '!link_qdc' => l(t('dublincore-qualifiers'), 'http://dublincore.org/documents/usageguide/qualifiers.shtml', array('absolute' => TRUE, 'attributes' => array('target' => '_blank'))),)),

    '#options' => $options,
    '#required' => TRUE,
    '#default_value' => array('dublin_core', 'qualified_dublin_core'),
  );

  $form['continue'] = array(
    '#type' => 'submit',
    '#value' => st('Continue'),
  );

  return $form;
}

/**
 * Process the submit of the metadata selection form.
 */
function mediamosa_profile_metadata_support_form_submit(&$form, &$form_state) {
  // Get the values from the submit.
  $values = $form_state['values'];

  // List of our metadata modules.
  $to_enable = array(
    'dublin_core' => 'mediamosa_metadata_dc',
    'qualified_dublin_core' => 'mediamosa_metadata_qdc',
    'asset' => 'mediamosa_metadata_asset',
    'czp' => 'mediamosa_metadata_czp',
  );

  // Enable the metadata modules that where selected.
  foreach ($to_enable as $type => $module) {
    if (!empty($values['metadata_support'][$type]) && $values['metadata_support'][$type] == $type) {
      module_enable(array($module));
    }
  }
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function system_form_install_settings_form_alter(&$form, $form_state, $form_id) {
  // Set default for site name field.
  $form['intro'] = array(
    '#weight' => -1,
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#title' => st('Setting up database'),
    '#description' => st("<p>We advice using a MySQL 5.1 compliant version or higher, such as !mariadb, !percona or !mysql.</p>
   <p>Follow the instructions on !create-database if you need help creating a database.<p>", array(
     '!mysql' => l(t('MySQL'), 'http://mysql.com', array('absolute' => TRUE)),
     '!mariadb' => l(t('MariaDB'), 'http://mariadb.org', array('absolute' => TRUE)),
     '!percona' => l(t('Percona'), 'http://www.percona.com/', array('absolute' => TRUE)),
     '!postgresql' => l(('PostgreSQL'), 'http://www.postgresql.org', array('absolute' => TRUE)),
     '!create-database' => l(('www.drupal.org create-database'), 'https://drupal.org/documentation/install/create-database', array('absolute' => TRUE)),
   )),
  );
  unset($form['driver']['#options']['pgsql']);
  unset($form['driver']['#options']['sqlite']);
}

/**
 * Implements hook_form_alter().
 */
function system_form_install_configure_form_alter(&$form, &$form_state, $form_id) {
  $form['site_information']['site_name']['#default_value'] = 'MediaMosa';
  $form['site_information']['site_mail']['#default_value'] = 'webmaster@' . $_SERVER['SERVER_NAME'];
  $form['admin_account']['account']['name']['#default_value'] = 'admin';
  $form['admin_account']['account']['mail']['#default_value'] = 'admin@' . $_SERVER['SERVER_NAME'];
}

/**
 * Implements hook_form().
 *
 * Show a checklist of the installation.
 */
function mediamosa_profile_php_settings_form($form, &$form_state, &$install_state) {

  // Requirements for PHP modules.
  $php_modules = mediamosa_profile::requirements_php_modules();
  $form['requirements']['php_modules']['title'] = array(
    '#markup' => '<h1>' . st('PHP Modules') . '</h1>',
  );
  $form['requirements']['php_modules']['requirements'] = array(
    '#markup' => theme('status_report', array('requirements' => $php_modules['requirements'])),
  );

  // Required installed 3rd party programs.
  $installed_programs = mediamosa_profile::requirements_installed_programs();
  $form['requirements']['installed_programs']['title'] = array(
    '#markup' => '<h1>' . st('Installed programs') . '</h1>',
  );
  $form['requirements']['installed_programs']['requirements'] = array(
    '#markup' => theme('status_report', array('requirements' => $installed_programs['requirements'])),
  );

  // Required PHP settings.
  $php_settings = mediamosa_profile::requirements_php_settings();
  $form['requirements']['php_settings']['title'] = array(
    '#markup' => '<h1>' . st('PHP variables / Settings') . '</h1>',
  );
  $form['requirements']['php_settings']['requirements'] = array(
    '#markup' => theme('status_report', array('requirements' => $php_settings['requirements'])),
  );

  // Check for errors.
  if ($php_modules['errors'] + $installed_programs['errors'] + $php_settings['errors']) {
    $form['requirements']['errors']['text'] = array(
      '#markup' => '<p><b>' . st("Solve the reported problems and press 'continue' to continue.") . '</b></p>',
    );
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => st('Continue'),
  );

  return $form;
}

/**
 * Implements hook_validate().
 */
function mediamosa_profile_php_settings_form_validate($form, &$form_state) {
  // Get the requirements.
  $php_modules = mediamosa_profile::requirements_php_modules();
  $installed_programs = mediamosa_profile::requirements_installed_programs();
  $php_settings = mediamosa_profile::requirements_php_settings();

  // Check for errors.
  if ($php_modules['errors'] + $installed_programs['errors'] + $php_settings['errors']) {
    form_set_error('foo', st('Solve the reported problems below before you continue. You can ignore the (yellow) warnings.'));
  }
}

/**
 * Get the mount point.
 */
function mediamosa_profile_storage_location_form() {
  $form = array();

  // Default mount pount.
  $mount_point = variable_get('mediamosa_current_mount_point', '/srv/mediamosa');

  $form['description'] = array(
    '#markup' => '<p><b>' . st('The MediaMosa mount point is a shared directory where all mediafiles are stored. On a multi-server setup, this mount point needs to be available for all servers (i.e. through NFS)') . '</b></p>',
  );

  $form['current_mount_point'] = array(
    '#type' => 'textfield',
    '#title' => t('MediaMosa SAN/NAS Mount point'),
    '#description' => st('Make sure the webserver user has write access to the MediaMosa SAN/NAS mount point.'),
    '#required' => TRUE,
    '#default_value' => $mount_point,
  );

  $form['continue'] = array(
    '#type' => 'submit',
    '#value' => st('Continue'),
  );

  return $form;
}

/**
 * Implements hook_validate().
 */
function mediamosa_profile_storage_location_form_validate($form, &$form_state) {
  // Get the mount point.
  $mount_point = $form_state['values']['current_mount_point'];

  if (trim($mount_point) == '') {
    form_set_error('current_mount_point', t("The mount point can't be empty."));
  }
  elseif (!file_exists($mount_point)) {
    form_set_error('current_mount_point', t("The mount point ('@mount_point') could not be found.", array('@mount_point' => $mount_point)));
  }
  elseif (!is_dir($mount_point)) {
    form_set_error('current_mount_point', t("The mount point ('@mount_point') is not a directory.", array('@mount_point' => $mount_point)));
  }
  elseif (!is_writable($mount_point)) {
    form_set_error('current_mount_point', t("The mount point ('@mount_point') is not writeable for the webserver (@server_software).", array('@server_software' => $_SERVER['SERVER_SOFTWARE'], '@mount_point' => $mount_point)));
  }
}

/**
 * Implements hook_submit().
 */
function mediamosa_profile_storage_location_form_submit($form, &$form_state) {
  // Get the mount point and make sure it ends with '/'.
  $mount_point = mediamosa_profile::trim_uri($form_state['values']['current_mount_point']);

  // Set our variables.
  variable_set('mediamosa_current_mount_point', $mount_point);
  variable_set('mediamosa_current_mount_point_temporary', $mount_point . 'data/transcode/');
  variable_set('mediamosa_current_mount_point_transition', $mount_point . 'data/transition/');

  // Profile does not does not handle Windows installations.
  variable_set('mediamosa_current_mount_point_windows', '\\\\');
  variable_set('mediamosa_current_mount_point_temporary_windows', '\\\\');
  variable_set('mediamosa_current_mount_point_transition_windows', '\\\\');

  // Setup the mountpoint.
  mediamosa_profile::setup_mountpoint();
}

/**
 * Implements hook_form().
 *
 * Information about cron, apache and migration.
 */
function mediamosa_profile_cron_settings_form() {
  $form = array();

  // Get the server name.
  $server_name = mediamosa_profile::get_server_name();
  if (variable_get('mediamosa_apache_setting') == 'simple') {
    $server_name = 'localhost';
  }

  // Cron.
  $form['cron'] = array(
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#title' => st('Cron setup'),
    '#description' => t('The cron will be used trigger MediaMosa every minute for background jobs. The
setup for cron is required for MediaMosa to run properly. On a multiple server
setup, the cron must be installed on one server only, for example on the job schedular server.'),
  );

  $form['cron']['crontab_text'] = array(
    '#markup' => st('<h5>Add a crontab entry</h5>Modify your cron using crontab, this will run the script every minute:<p><code>crontab -e</code></p><p>Add this line at the bottom:</p>'),
  );

  $form['cron']['crontab'] = array(
    '#markup' => '<p><pre>* * * * * /usr/bin/wget -O - -q -t 1 --header="Host: ' . $server_name . '" http://localhost' . url('') . 'cron.php?cron_key=' . variable_get('cron_key', '') . '</pre></p>',
    '#rows' => 6,
  );

  $form['continue'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );

  // Reset static for 'file_get_stream_wrappers' to solve issue of invoke
  // stream_wrappers for mediamosa in drush
  drupal_static_reset('file_get_stream_wrappers');

  // Flush all.
  drupal_flush_all_caches();

  return $form;
}

/**
 * Implements hook_form().
 *
 * Information about cron, apache and migration.
 */
function mediamosa_profile_apache_settings_form() {
  $form = array();

  // Get the server name.
  $server_name = mediamosa_profile::get_server_name();
  $mount_point = variable_get('mediamosa_current_mount_point', '');

  $apache_settings_local = st("This setup is for installation of MediaMosa on 1 server and 1 domain.
  Can be used for testing setups, but also for a small server deploy. MediaMosa can be installed on a subdirectory
  (http://domain/mediamosa), or in the document root (http://domain).
  <p><li>Add the following lines to your default apache definition:</p>
    <pre>" . htmlentities("
    # Media
    Alias !rel_directorymedia !mount_point/media
    <Directory !mount_point/media>
      Options FollowSymLinks
      AllowOverride All
      Order deny,allow
      Allow from All
    </Directory>

    <IfModule mod_php5.c>
        php_admin_value post_max_size 2008M
        php_admin_value upload_max_filesize 2000M
        php_admin_value memory_limit 128M
    </IfModule>") . '</pre>
<p>The media alias is the (streaming) link to a mediafile needed to view files, the php settings are needed to allow more than default sizes upload. For more information about upload limits, see also !uploadlimits.</p>
<p><li>Restart your Apache:</p><p><code>sudo /etc/init.d/apache2 restart</code></p>
', array(
    '!mount_point' => mediamosa_profile::trim_uri($mount_point, ''),
    '!server_name_clean' => $server_name,
    '!document_root' => DRUPAL_ROOT,
    '!rel_directory' => url(),
    '!uploadlimits' => l(t('www.mediamosa.org/server-setup'), ''),
  ));

  $server_name_clean = $server_name;

  $apache_settings_adv = st("Multi-server setup with different DNS entries for a production or development setup.
<p><li>Insert the vhost setup below into the new file /etc/apache2/sites-available/<b>your-site</b>, where <b>your-site</b> is the name of your MediaMosa site:</p>
<pre>" . htmlentities("
<VirtualHost *:80>
    ServerName !server_name_clean
    ServerAlias admin.!server_name_clean www.!server_name_clean
    ServerAdmin webmaster@!server_name_clean
    DocumentRoot !document_root
    <Directory !document_root>
        Options FollowSymLinks MultiViews
        AllowOverride All
        Order allow,deny
        allow from all
    </Directory>

    ErrorLog /var/log/apache2/!server_name_clean_error.log
    CustomLog /var/log/apache2/!server_name_clean_access.log combined
    ServerSignature On

    Alias /server-status !document_root
    <Directory !document_root/serverstatus>
        SetHandler server-status
        Order deny,allow
        Deny from all
        Allow from 127.0.0.1
     </Directory>

    # Media
    Alias /media !mount_point/media
    <Directory !mount_point/media>
      Options FollowSymLinks
      AllowOverride All
      Order deny,allow
      Allow from All
    </Directory>
</VirtualHost>

<VirtualHost *:80>
    ServerName app1.!server_name_clean
    ServerAdmin webmaster@!server_name_clean
    DocumentRoot !document_root
    <Directory !document_root>
        Options FollowSymLinks MultiViews
        AllowOverride All
        Order allow,deny
        allow from all
    </Directory>

    ErrorLog /var/log/apache2/app1.!server_name_clean_error.log
    CustomLog /var/log/apache2/app1.!server_name_clean_access.log combined
    ServerSignature On
</VirtualHost>

<VirtualHost *:80>
    ServerName app2.!server_name_clean
    ServerAdmin webmaster@!server_name_clean
    DocumentRoot !document_root
    <Directory !document_root>
        Options FollowSymLinks MultiViews
        AllowOverride All
        Order allow,deny
        allow from all
    </Directory>

    ErrorLog /var/log/apache2/app2.!server_name_clean_error.log
    CustomLog /var/log/apache2/app2.!server_name_clean_access.log combined
    ServerSignature On
</VirtualHost>

<VirtualHost *:80>
    ServerName upload.!server_name_clean
    ServerAdmin webmaster@!server_name_clean
    DocumentRoot !document_root
    <Directory !document_root>
        Options FollowSymLinks MultiViews
        AllowOverride All
        Order allow,deny
        allow from all
    </Directory>

    <IfModule mod_php5.c>
        php_admin_value post_max_size 2008M
        php_admin_value upload_max_filesize 2000M
        php_admin_value memory_limit 128M
    </IfModule>

    ErrorLog /var/log/apache2/upload.!server_name_clean_error.log
    CustomLog /var/log/apache2/upload.!server_name_clean_access.log combined
    ServerSignature On
</VirtualHost>

<VirtualHost *:80>
    ServerName download.!server_name_clean
    ServerAdmin webmaster@!server_name_clean
    DocumentRoot !document_root
    <Directory !document_root>
        Options FollowSymLinks MultiViews
        AllowOverride All
        Order allow,deny
        allow from all
    </Directory>

    # Media
    Alias /media !mount_point/media
    <Directory !mount_point/media>
      Options FollowSymLinks
      AllowOverride All
      Order deny,allow
      Allow from All
    </Directory>

    ErrorLog /var/log/apache2/download.!server_name_clean_error.log
    CustomLog /var/log/apache2/download.!server_name_clean_access.log combined
    ServerSignature On
</VirtualHost>

<VirtualHost *:80>
    ServerName job1.!server_name_clean
    ServerAdmin webmaster@!server_name_clean
    DocumentRoot !document_root
    <Directory !document_root>
        Options FollowSymLinks MultiViews
        AllowOverride All
        Order allow,deny
        allow from all
    </Directory>

    ErrorLog /var/log/apache2/job1.!server_name_clean_error.log
    CustomLog /var/log/apache2/job1.!server_name_clean_access.log combined
    ServerSignature On
</VirtualHost>

<VirtualHost *:80>
    ServerName job2.!server_name_clean
    ServerAdmin webmaster@!server_name_clean
    DocumentRoot !document_root
    <Directory !document_root>
        Options FollowSymLinks MultiViews
        AllowOverride All
        Order allow,deny
        allow from all
    </Directory>

    ErrorLog /var/log/apache2/job2.!server_name_clean_error.log
    CustomLog /var/log/apache2/job2.!server_name_clean_access.log combined
    ServerSignature On
</VirtualHost>") . '
</pre><p><li>Enable the website:</p><p><code>sudo a2ensite <b>your-site</b></code></p>
<p><li>Restart Apache:</p><p><code>sudo /etc/init.d/apache2 restart</code></p>',
    array(
      '!server_name_clean' => $server_name_clean,
      '!document_root' => DRUPAL_ROOT,
      '!mount_point' => mediamosa_profile::trim_uri($mount_point, ''),
    )
  );

  $form['apache'] = array(
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#title' => t('Apache HTTP server setup'),
    '#description' => st("Choose a server setup. See !mediamosa.org/server-setup for more information
about server setups, and Apache/Nginx configurations. We recommend the Multiserver setup for
production websites. The single setup can be used for testing purposes, and
single machine deploys.",
       array('!mediamosa.org/server-setup' => l(t('mediamosa.org/server-setup'), 'http://www.mediamosa.org/server-setup'))
    ),
  );

  $form['apache']['localhost'] = array(
    '#type' => 'radios',
    '#options' => array(
      'simple' => '<b>' . t('Single server / domain setup.') . '</b>',
      'advanced' => '<b>' . t('Multiple server / domain setup.') . '</b>',
    ),
  );

  $form['apache']['local'] = array(
    '#type' => 'fieldset',
    '#title' => t('Single server / domain setup.'),
    '#states' => array(
      'visible' => array(
        ':input[name="localhost"]' => array('value' => 'simple'),
      ),
    ),
  );
  $form['apache']['local']['local_text'] = array(
    '#markup' => $apache_settings_local,
  );

  $form['apache']['multi'] = array(
    '#type' => 'fieldset',
    '#title' => t('Multi server/domain setup (production setup).'),
    '#states' => array(
      'visible' => array(
        ':input[name="localhost"]' => array('value' => 'advanced'),
      ),
    ),
  );
  $form['apache']['multi']['multi_text'] = array(
    '#markup' => $apache_settings_adv,
  );

  $form['continue'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );

  return $form;
}

/**
 * Implements hook_validate().
 */
function mediamosa_profile_apache_settings_form_validate($form, &$form_state) {
  if (!in_array($form_state['values']['localhost'], array('simple', 'advanced'))) {
    form_set_error('', t('You must choose a setup.'));
  }
}

/**
 * Implements hook_submit().
 */
function mediamosa_profile_apache_settings_form_submit($form, &$form_state) {
  // Check for https.
  $is_https = isset($_SERVER['HTTPS']) && strtolower($_SERVER['HTTPS']) == 'on';
  $scheme = $is_https ? 'https://' : 'http://';

  // Get current url.
  $server_name = mediamosa_profile::get_server_name();

  // Save the chosen install state.
  variable_set('mediamosa_apache_setting', ($form_state['values']['localhost'] == 'simple' ? 'simple' : 'advanced'));

  // Simple type of server? Then change default setup of advanced.
  if (variable_get('mediamosa_apache_setting') == 'simple') {
    // Simple URL for job and app.
    variable_set('mediamosa_jobscheduler_uri', $scheme . $server_name . '/');
    variable_set('mediamosa_cron_url_app', $scheme . $server_name . '/');

    // Get all mediamosa_server nodes nids, and update server_uri.
    $results = db_select('mediamosa_server', 'ms')
      ->fields('ms', array(mediamosa_server_db::NID))
      ->execute();

    // Walk through the server nodes and save the simple url.
    foreach ($results as $result) {
      $node = node_load($result->{mediamosa_server_db::NID});
      $node->{mediamosa_server_db::SERVER_URI} = $scheme . $server_name . '/';
      node_save($node);
    }
  }
  else {
    // Advanced URLs.
    variable_set('mediamosa_jobscheduler_uri', $scheme . 'job1.' . $server_name . '/');
    variable_set('mediamosa_cron_url_app', $scheme . 'app1.' . $server_name . '/');
  }
}

function mediamosa_profile_domain_usage_form() {
  // Check for https.
  $is_https = isset($_SERVER['HTTPS']) && strtolower($_SERVER['HTTPS']) == 'on';
  $scheme = $is_https ? 'https://' : 'http://';

  $form = array();

  $form['domain'] = array(
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#title' => t("Your domain, MediaMosa and Drupal's multiple sites"),
  );

  $form['domain']['apache_options'] = array(
    '#markup' => st(
    "Your MediaMosa setup is using the '!host' DNS name. All REST interfaces, download and upload URLs use these subdomains in your current setup. We use these subdomains as example; <ul>
    <li>!scheme!host/ or !schemeadmin.!host/ as administration front-end.</li>
    <li>!schemeapp1.!host/ is an application REST interface.</li>
    <li>!schemeapp2.!host/ is an application REST interface.</li>
    <li>!schemejob1.!host/ is an job REST interface used for transcoding and other job handeling tasks.</li>
    <li>!schemejob2.!host/ is an job REST interface used for transcoding and other job handeling tasks.</li>
    <li>!schemedownload.!host/ is used for downloading files from MediaMosa.</li>
    <li>!schemeupload.!host/ is used for uploading files to MediaMosa.</li>
    </ul>
    In the directory /sites in your MediaMosa installation, each of these DNS names do also exists as an directory, in each an example default.settings.php.<br />
    <br />
    It's important to know when using multiple subdomains for MediaMosa interfaces that each need an unique settings.php where at the end of the file an indentifier is used to indentify the interface. See our example default.settings.php files in each directory and notice the '\$conf['mediamosa_installation_id']' at the end of each (default.)settings.php file.<br />
    <br />
    Using multiple subdomains allows you to scale your MediaMosa installation to use more APP or more JOB servers.<br />
    <br />
    For more information how to setup our multiple subdomains read the !link on the Drupal website.",
    array(
      '!host' => mediamosa_profile::get_host(),
      '!link' => l('Advanced and multisite installation', 'http://drupal.org/node/346385', array('attributes' => array('target' => '_blank'), 'absolute' => TRUE, 'external' => TRUE)),
      '!scheme' => $scheme,
    )),
  );

  $form['continue'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );

  return $form;
}
