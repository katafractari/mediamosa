<?php
/**
 * @file
 * The response connector allows you to do REST calls under all APPs and is
 * used by maintenance. Do not use as normal REST call connection class, use
 * mediamosa_connector instead, which is application session based.
 *
 * This class is only intended for MediaMosa internal workings for maintenance
 * and administration modules. Again, better to use the mediamosa_connector.
 */

class mediamosa_response_connector extends mediamosa_response {

  // ---------------------------------------------------------------- Functions.
  /**
   * Execute REST call.
   *
   * @param $uri
   *  URI of rest call.
   * @param $method
   *  Method of REST call.
   * @param $params
   *  GET/POST parameters.
   */
  public static function static_do_restcall($uri, $method = 'GET', array $params = array()) {

    // Just to save time.
    $app_access = &drupal_static(__CLASS__ . '::' . __FUNCTION__, array());

    // If we don't provide any app_ids, then we'll need to limit the access.
    if (empty($params['app_id'])) {
      if (user_access(mediamosa_permission::ADMIN_MEDIAMOSA)) {
        // Because the rule is the 1st is always the leading application,
        // we don't have one here, so use 0 instead.
        $params['app_id'] = array_merge(array(0), mediamosa_app::get_all_apps(array(mediamosa_app_db::APP_ID), mediamosa_app_db::APP_ID)->fetchCol());
      }
      else {
        if (empty($app_access)) {
          // Add the main app as default (is ignored, but now the $app_access
          // is no longer empty;
          $app_access = array(0);

          // Now get all the apps.
          $apps = mediamosa_app::get_all_apps_cached();

          // Now we are not MediaMosa admin, we'll limit the access based on the
          // app roles.
          foreach ($apps as $app) {
            // Now check them one by one till we know access, when access, add
            // the app id.
            if (_mediamosa_user_access(mediamosa_permission::ADMIN_MEDIAMOSA . '|app|' . $app[mediamosa_app_db::APP_ID])) {
              $app_access[] = $app[mediamosa_app_db::APP_ID];
            }
          }
        }

        // Limit the access to its apps.
        $params['app_id'] = $app_access;
      }
    }

    // Create response object.
    $mediamosa_reponse = new mediamosa_response_connector();

    // Do REST call.
    return $mediamosa_reponse->do_restcall($uri, $method, $params);
  }

  /**
   * Execute a REST call. Report to drupal if an error was found.
   *
   * @param string $uri
   *  The URI of the REST call.
   * @param string $method
   *  The Method (POST/GET/PUT).
   * @param array $data
   *  Array name => value.
   */
  public static function static_do_restcall_drupal($uri, $method = 'GET', array $params = array()) {

    // Call REST response class to generate output.
    $result = self::static_do_restcall($uri, $method, $params);

    if ($result['header']['request_result'] != self::SUCCESS) {
      drupal_set_message($uri);
      drupal_set_message($result['header']['request_result_id'] . ': ' . $result['header']['request_result_description'], 'error');
    }

    return $result;
  }

  /**
   * Test if the result is okay. (empty result is not okay).
   */
  public static function static_result_okay($result) {
    return (!empty($result['header']['request_result_id']) && (int) $result['header']['request_result_id'] == mediamosa_error::ERRORCODE_OKAY) ? TRUE : FALSE;
  }

  // ---------------------------------------------------------------- Functions.
  /**
   * Do a request, without using HTTP.
   *
   * @param string $uri
   *  The REST call uri, like 'asset', 'version' etc.
   * @param array $params
   *  name => value.
   */
  public function do_restcall($uri, $method = 'GET', array $params = array()) {

    // Store uri here.
    $this->request_uri = $uri;
    $this->request_method = $method;

    // App ID.
    $app_id = self::$environment[self::ENV_APP_ID];
    $app_ids = self::$environment[self::ENV_APP_IDS];
    self::$environment[self::ENV_APP_ID] = is_array($params['app_id']) ? reset($params['app_id']) : $params['app_id'];
    self::$environment[self::ENV_APP_IDS] = is_array($params['app_id']) ? $params['app_id'] : array($params['app_id']);

    // ok reset the header.
    $this->reset();

    // Lets fool the interface that we are an app interface.
    global $conf;
    $conf['mediamosa_app'] = TRUE;

    // Replace the global mediamosa_response with us.
    $current_mediamosa_response = mediamosa::get($this);
    $GET = $_GET;
    $POST = $_POST;

    $_GET = array('q' => (isset($_GET['q']) ? $_GET['q'] : ''));
    $_POST = array();

    try {
      // Set the global params, so REST will use that.
      $GLOBALS['mediamosa_params'] = $params;

      // Create the REST interface class.
      $this->mediamosa_rest = new mediamosa_rest();

      // Find the rest call.
      $this->rest_call = $this->mediamosa_rest->match_call($uri, $method);
      if (!$this->rest_call) {
        throw new mediamosa_exception_error(mediamosa_error::HTTP_NOT_FOUND, array(
          '@uri' => print_r($uri, TRUE),
          '@method' => ($method ? print_r($method, TRUE) : '-'),
          '@params' => ($params ? print_r($params, TRUE) : '-'),
        ));
      }

      // Ok make it external so we always get passed the app authorization.
      $this->rest_call[mediamosa_rest_call::ACCESS] |= mediamosa_rest_call::ACCESS_EXTERNAL;
      $this->rest_call[mediamosa_rest_call::BEHAVE_AS_EXTERNAL] = TRUE;

      // Now execute the call.
      $this->process_call();
    }
    catch (mediamosa_exception $e) {
      $e->mediamosa_exception_rest_response($this);
    }
    catch (PDOException $e) {
      mediamosa_debug::error_log('PDOException caught; ' . $e->getMessage() . ', trace; ' . $e->getTraceAsString(), $e->getFile(), $e->getLine());
      $this->set_result(self::ERROR, mediamosa_error::HTTP_INTERNAL_SERVER_ERROR, 'PDOException caught; ' . $e->getMessage() . ",\ntrace; " . $e->getTraceAsString() . ",\nQuery string; " . $e->query_string . ",\nArgs; " . print_r($e->args, TRUE));
      try {
        mediamosa_watchdog::log_export(strtr('PDOException caught file @file at line @line; @message, trace; @trace.', array('@file' => $e->getFile(), '@line' => $e->getLine(), '@message' => $e->getMessage(), '@trace' => $e->getTraceAsString())));
      }
      catch (Exception $e) {
        // ignore.
      }
    }
    catch (Exception $e) {
      // Store as error.
      $this->set_result(self::ERROR, $e->getCode(), $e->getMessage());
      try {
        mediamosa_watchdog::log_export(strtr('Exception caught file @file at line @line; @message, trace; @trace.', array('@file' => $e->getFile(), '@line' => $e->getLine(), '@message' => $e->getMessage(), '@trace' => $e->getTraceAsString())));
      }
      catch (Exception $e) {
        // ignore.
      }
    }

    // Generate output PHP array.
    $output = $this->generate_array();

    // Restore.
    mediamosa::get($current_mediamosa_response);
    $_GET = $GET;
    $_POST = $POST;
    unset($GLOBALS['mediamosa_params']);
    self::$environment[self::ENV_APP_ID] = $app_id;
    self::$environment[self::ENV_APP_IDS] = $app_ids;

    // Return XML.
    return $output;
  }
}
