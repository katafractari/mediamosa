<?php
/**
 * @file
 * main storage class.
 */

abstract class mediamosa_storage {
  // ------------------------------------------------------------------- Consts.
  const STORAGE_FOLDER_TICKET = 'ticket';

  // --------------------------------------------------------------------- Vars.
  /**
   * The storage profile array.
   *
   * @var mediamosa_storage
   */
  protected $storage_profile;

  /**
   * The applications linked to profile for mediafiles.
   *
   * @var mediamosa_storage_app
   */
  protected $storage_profile_apps;

  /**
   * The applications linked to profile for stills.
   *
   * @var mediamosa_storage_app
   */
  protected $storage_profile_apps_stills;

  // ----------------------------------------------------- Functions (abstract).
  /**
   * Get the default scheme (without the '://').
   *
   * @param boolean $is_still
   *   Is mount point for still?
   *
   * @return string
   *   The uri of the moint point to store.
   */
  public abstract function get_scheme($is_still = FALSE);

  /**
   * Return the storage options available for the storage type
   *
   * @return
   *   Array of available storage options
   */
  public abstract function get_storage_options();

  /**
   * Return alle required upload parameters for the storage type
   * which allow a frontend application to build the correct upload form
   *
   * $param $app_id
   *   Application ID.
   *
   * @param $user_id
   *   User ID
   * @param $group_id
   *   Group ID
   *
   * @return
   *   Array with upload parameters
   */
  //public abstract function get_upload_parameters($app_id, $user_id, $group_id);

  // -------------------------------------------------------------- Constructor.
  /**
   * Main constructor
   *
   * @param array $storage_profile
   *   Storage profile.
   */
  public function __construct(array $storage_profile) {
    $this->storage_profile = $storage_profile;

    // Get all storage profiles apps.
    $this->load_all_storage_profile_apps();
  }

  // ------------------------------------------------------------------ Methods.
  /**
   * Return with uri of the profile mount point.
   *
   * Unless local mount point, the uri must include $storage_profile::ID and
   * $storage_profile_app::PATH or $storage_profile_app::PATH_STILLS.
   *
   * @param integer $storage_profile_id
   *   The storage profile ID.
   * @param string $path
   *   The storage path.
   * @param boolean $is_still
   *   Is mount point for still?
   *
   * @return string
   *   The uri of the storage profile.
   */
  public function build_mount_point_uri($storage_profile_id, $path, $is_still = FALSE) {
    return self::trim_uri($this->get_scheme($is_still) . '://' . $storage_profile_id) . $path;
  }

  /**
   * Is storage object local or external?
   *
   * @return boolean
   *   Return TRUE when all data is stored on local storage.
   */
  public function is_local() {
    return TRUE;
  }

  /**
   * Load all linked storage_profile_apps.
   */
  protected function load_all_storage_profile_apps() {
    // Build the query.
    $storage_profile_apps = mediamosa_db::db_select(mediamosa_storage_profile_app_db::TABLE_NAME, 'spa')
      ->condition(db_or()
        ->condition(mediamosa_storage_profile_app_db::PROFILE_ID, $this->storage_profile[mediamosa_storage_profile_db::ID])
        ->condition(mediamosa_storage_profile_app_db::PROFILE_ID_STILLS, $this->storage_profile[mediamosa_storage_profile_db::ID])
      )
      ->fields('spa')
      ->execute();

    // Storage profiles for mediafiles.
    $this->storage_profiles_apps = array();

    // Storage profiles for stills.
    $this->storage_profiles_apps_stills = array();

    // Store them.
    foreach ($storage_profile_apps as $storage_profile_app) {
      if ($storage_profile_app[mediamosa_storage_profile_app_db::PROFILE_ID] == $this->storage_profile[mediamosa_storage_profile_db::ID]) {
        $this->storage_profile_apps[$storage_profile_app[mediamosa_storage_profile_app_db::APP_ID]] = $storage_profile_app;
      }

      if ($storage_profile_app[mediamosa_storage_profile_app_db::PROFILE_ID_STILLS] == $this->storage_profile[mediamosa_storage_profile_db::ID]) {
        $this->storage_profiles_apps_stills[$storage_profile_app[mediamosa_storage_profile_app_db::APP_ID]] = $storage_profile_app;
      }
    }
  }

  // ---------------------------------------------------------------- Functions.
  /**
   * Is the storage file system path specified?
   *
   * @return boolean
   *   Returns TRUE when storage file system path has been specified.
   */
  public static function is_storage_setup() {
    return mediamosa_io::realpath('mediamosa-storage://') != '';
  }

  /**
   * Return the storage scheme uri.
   *
   * @return string
   *   The storage scheme uri.
   */
  public static function get_uri() {
    return 'mediamosa-storage://';
  }

  /**
   * Return the translated real path of the storage.
   *
   * @return string
   *   The storage real path.
   */
  public static function get_realpath() {
    return mediamosa_io::realpath('mediamosa-storage://');
  }

  /**
   * Return the simpletest storage scheme uri during simpletest.
   *
   * @return string
   *   The simpletest storage scheme uri.
   */
  public static function get_uri_simpletest() {
    return 'mediamosa-storage-simpletest://';
  }

  // ------------------------------------------------------- Storage mountpoint.
  /**
   * Get the mount point uri based on the storage profile and path.
   *
   * Both these values are found in the mediamosa_storage_profile_app table.
   *
   * @param array $storage_profile_id
   *   The storage profile ID.
   * @param string $path
   *   The (prefix) path on the uri.
   * @param boolean $is_still
   *   TRUE when mediafile is still.
   *
   * @return string
   *   The local moint point uri.
   */
  public static function get_mount_point_uri($storage_profile_id, $path, $is_still = FALSE) {
    // Need parent storage profile.
    $storage_profile = mediamosa_storage_profile::get($storage_profile_id);

    // Get uri.
    return self::get_mount_point_uri_by_storage_profile($storage_profile, $path, $is_still);
  }

  /**
   * Get the mount point uri based on the storage profile and path.
   *
   * Both these values are found in the mediamosa_storage_profile_app table.
   *
   * @param array $storage_profile_app
   *   The storage profile app (link between app and storage profile).
   * @param boolean $is_still
   *   TRUE when mediafile is still.
   *
   * @return string
   *   The local moint point uri.
   */
  public static function get_mount_point_uri_by_storage_profile_app(array $storage_profile_app, $is_still = FALSE) {
    // Get right path.
    $path = $is_still ? $storage_profile_app[mediamosa_storage_profile_app_db::PATH_STILLS] : $storage_profile_app[mediamosa_storage_profile_app_db::PATH];

    return self::get_mount_point_uri($storage_profile_app[mediamosa_storage_profile_app_db::PROFILE_ID], $path, $is_still);
  }

  /**
   * Get the mount point uri based on the storage profile app array.
   *
   * @param array $storage_profile
   *   The storage profile.
   * @param string $path
   *   The (prefix) path on the uri.
   * @param boolean $is_still
   *   TRUE when mediafile is still.
   *
   * @return string
   *   The local moint point uri.
   */
  public static function get_mount_point_uri_by_storage_profile(array $storage_profile, $path, $is_still = FALSE) {
    // Create the storage instance.
    $mediamosa_storage = mediamosa_storage_profile::create_storage_instance_by_profile($storage_profile);

    // Build the local mount point.
    return $mediamosa_storage->build_mount_point_uri($storage_profile[mediamosa_storage_profile_db::ID], $path, $is_still);
  }

  // -------------------------------------------------------------- Storage URI.
  /**
   * Create the uri of the file where the mediafile will be stored.
   *
   * The storage uri can be local or external (e.g. S3). This function can not
   * be used to get the current URI location of the mediafile (use
   * mediamosa_storage::get_mediafile_uri() instead).
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $mediafile_id
   *   The mediafile ID.
   * @param boolean $is_still
   *   TRUE when mediafile is still.
   *
   * @return string
   *   The storage destination uri.
   */
  public static function create_storage_uri($app_id, $mediafile_id, $is_still = FALSE) {
    // Get the storage profile app based on the application ID.
    $storage_profile_app = mediamosa_storage_profile_app::get_app_storage_profile($app_id);

    // Build up the mount point uri.
    $mount_point_uri = self::get_mount_point_uri_by_storage_profile_app($storage_profile_app, $is_still);

    // Create tmp wrapper using the mount pount uri (which is not complete).
    $wrapper = mediamosa_io::require_stream_wrapper_instance_by_uri($mount_point_uri);

    // Only our class has create_storage_uri.
    if ($wrapper instanceof mediamosa_io_streamwrapper) {
      return $wrapper->create_storage_uri($mediafile_id);
    }

    // Return the complete uri.
    return self::trim_uri($mount_point_uri, '') . mediamosa_io_streamwrapper::create_mediafile_path($mediafile_id);
  }

  // -------------------------------------------------------------------- Local.
  /**
   * Create the local mount point uri.
   *
   * This function provides the mountpoint of the first uri of the new created
   * mediafiles on local file system. Is stored in the sannas_mount_point field.
   *
   * @todo: need to return local moint point (always).
   *
   * @param integer $app_id
   *   The application ID.
   * @param boolean $is_still
   *   TRUE when mediafile is still.
   *
   * @return string
   *   The local moint point uri.
   */
  public static function create_local_mount_point_uri($app_id, $is_still = FALSE) {
    // Get the storage profile app based on the application ID.
    $storage_profile_app = mediamosa_storage_profile_app::get_app_storage_profile($app_id);

    // Build up the mount point uri.
    $local_uri = self::get_mount_point_uri_by_storage_profile_app($storage_profile_app, $is_still);

    // See if its local.
    //
    // Build up the stream wrapper.
    $local_streamwrapper = mediamosa_io::require_stream_wrapper_instance_by_uri($local_uri);

    if ($local_streamwrapper->is_local()) {
      return $local_uri;
    }

    // Not local, then revert back to default storage.
    return self::get_uri_data();
  }

  /**
   * Create the complete file path for new mediafiles.
   *
   * The path and filename returned are intended for creation of the new file.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $mediafile_id
   *   The mediafile ID.
   * @param boolean $is_still
   *   TRUE when mediafile is still.
   *
   * @return string
   *   The local mediafile uri.
   */
  public static function create_local_mediafile_uri($app_id, $mediafile_id, $is_still = FALSE) {
    return self::trim_uri(self::create_local_mount_point_uri($app_id, $is_still), '') . mediamosa_io_streamwrapper::create_mediafile_path($mediafile_id);
  }

  /**
   * Return the local path location of the mediafile.
   *
   * Need to return the path without $mediafile_id[0] . '/' . $mediafile_id.
   * Also used by simpletests.
   *
   * @param array|string $mediafile
   *   Either the mediafile ID or the mediafile array.
   *
   * @return string
   *   The uri without $mediafile_id[0] . '/' . $mediafile_id
   */
  public static function get_uri_mediafile_path($mediafile) {
    // Get the mediafile uri.
    $source_uri = self::get_uri_mediafile($mediafile);

    // Build up the stream wrapper.
    $source_streamwrapper = mediamosa_io::require_stream_wrapper_instance_by_uri($source_uri);

    return $source_streamwrapper->get_uri_mount_point();
  }

  /**
   * Return the local path location of the mediafile.
   *
   * Used by LUA scripts:
   * Need to return the path without $mediafile_id[0] . '/' . $mediafile_id.
   * Also used by simpletests.
   *
   * @param array|string $mediafile
   *   Either the mediafile ID or the mediafile array.
   *
   * @return string
   *   The path without $mediafile_id[0] . '/' . $mediafile_id.
   */
  public static function get_local_mediafile_path($mediafile) {
    // Get the mediafile uri.
    $source_uri = self::get_uri_mediafile($mediafile);

    // Build up the stream wrapper.
    $source_streamwrapper = mediamosa_io::require_stream_wrapper_instance_by_uri($source_uri);

    // If the mediafile is not local, then return default mount point for
    // transition links.
    if (!$source_streamwrapper->is_local()) {
      return self::get_realpath_transition_links();
    }

    // LUA can use the transition location (mediafile_id[0]/mediafile_id/md5) so
    // use links version instead. Although it will point to the latest
    // registered transition.
    if ($source_streamwrapper instanceof mediamosa_io_streamwrapper_local_transition) {
      return mediamosa_storage::get_realpath_transition_links();
    }

    return $source_streamwrapper->get_realpath_mount_point();
  }

  /**
   * Process a new mediafile after upload or transcoding.
   *
   * Will move the file to its final storage location. Expect the mediafile ID
   * to exist as mediafile entry in database.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $mediafile_id
   *   The mediafile ID of the new file.
   * @param boolean $is_new
   *   Will alter the sannas location to transition if TRUE.
   */
  public static function relocate_mediafile($app_id, $mediafile_id, $is_new) {
    // Need (new) destination uri.
    $destination_uri = mediamosa_storage::create_storage_uri($app_id, $mediafile_id);

    // Find the stream wrapper.
    $mediamosa_io_streamwrapper_destination = mediamosa_io::require_stream_wrapper_instance_by_uri($destination_uri);

    // If destination is local, then we do nothing.
    if ($mediamosa_io_streamwrapper_destination->is_local()) {
      return;
    }

    // Move file to transition. Returns the new location.
    $destination_uri = mediamosa_storage_transition::mediafile_local_to_transition($mediafile_id);

    // New location for mediafile is transition.
    $mediamosa_io_streamwrapper_destination = mediamosa_io::require_stream_wrapper_instance_by_uri($destination_uri);

    // Update mount point with transition location, but use the link to the
    // file.
    if ($is_new) {
      mediamosa_storage::mediafile_update_mointpoint($mediafile_id, mediamosa_storage::get_uri_transition_links());
    }

    // Transition can not be the final location. So create a upload job to
    // its final storage.
    $mediafile = mediamosa_asset_mediafile::get($mediafile_id);

    // Create the job.
    mediamosa_job::create_job_media_upload($app_id, $mediafile[mediamosa_asset_mediafile_db::OWNER_ID], FALSE, $mediafile_id);
  }

  // -------------------------------------------------- /data related functions.
  /**
   * Return the uri of the (data) storage.
   *
   * @return string
   *   The storage real path.
   */
  public static function get_uri_data() {
    return 'mediamosa-storage-data://';
  }

  /**
   * Return the translated real path of the (data) storage.
   *
   * @return string
   *   The storage real path.
   */
  public static function get_realpath_data() {
    return mediamosa_io::realpath('mediamosa-storage-data://');
  }

  /**
   * Return the mediafile uri.
   *
   * @param array|string $mediafile
   *   Either the mediafile ID or the mediafile array.
   *
   * @return string
   *   The location of the data location.
   *
   * @see get_realpath_mediafile()
   */
  public static function get_uri_mediafile($mediafile) {
    // MediaFile ID?
    if (is_string($mediafile)) {
      $mediafile = mediamosa_asset_mediafile::must_exists($mediafile);
    }

    // If no mount point, then always assign default.
    if (empty($mediafile[mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT])) {
      $mediafile[mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT] = self::get_uri_data();
    }

    // Create tmp wrapper.
    $wrapper = mediamosa_io::require_stream_wrapper_instance_by_uri($mediafile[mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT]);
    if ($wrapper instanceof mediamosa_io_streamwrapper) {
      return $wrapper->get_uri_mediafile($mediafile);
    }

    return self::trim_uri($mediafile[mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT], '') . mediamosa_io_streamwrapper::create_mediafile_path($mediafile[mediamosa_asset_mediafile_db::ID]);
  }

  /**
   * Return the mediafile location.
   *
   * Will throw exception when file is not local or does not exist.
   *
   * @param array|string $mediafile
   *   Either the mediafile ID or the mediafile array.
   *
   * @return string
   *   The location of the the data location.
   */
  public static function get_realpath_mediafile($mediafile) {
    return mediamosa_io::realpath(self::get_uri_mediafile($mediafile));
  }

  /**
   * Check if physical file is found.
   *
   * @param array|string $mediafile
   *   The mediafile to check. Either ID or mediafile array.
   * @param bool $include_external
   *   If file located on external source, then skip the check. Provide TRUE to
   *   check file, even when on external source.
   * @param bool $fatal
   *   File not exists will result in exception. Default TRUE.
   *
   * @param bool
   *   Returns TRUE when file exists. When $fatal = TRUE, exception will be
   *   thrown when file does not exist else FALSE is returned.
   *
   * @throws mediamosa_exception_error_file_not_found
   */
  public static function file_must_exist($mediafile, $include_external = FALSE, $fatal = TRUE) {
    // Check if still file exists.
    $uri = self::get_uri_mediafile($mediafile);

    if ( ($include_external || mediamosa_io::is_local($uri)) && !mediamosa_io::file_exists($uri)) {
      if ($fatal) {
        throw new mediamosa_exception_error_file_not_found(array('@filename' => mediamosa_io::realpath_safe($uri)));
      }
      return FALSE;
    }

    return TRUE;
  }

  // -------------------------------------------- /transition related functions.
  /**
   * Return the transition folder.
   *
   * @return string
   *   The location of the transition folder; data/transition.
   *
   * @see get_realpath_transition()
   */
  protected static function get_uri_transition() {
    return 'mediamosa-storage-transition://';
  }

  /**
   * Return the translated tmp transcode folder.
   *
   * @return string
   *   The location of the transition folder; data/transition.
   */
  public static function get_realpath_transition() {
    return mediamosa_io::realpath(self::get_uri_transition());
  }

  // -------------------------------------- /transition/links related functions.
  /**
   * Return the transition folder.
   *
   * @return string
   *   The location of the transition folder; data/transition.
   *
   * @see get_realpath_transition()
   */
  public static function get_uri_transition_links() {
    return 'mediamosa-storage-transition-links://';
  }

  /**
   * Return the translated tmp transcode folder.
   *
   * @return string
   *   The location of the transition folder; data/transition.
   */
  public static function get_realpath_transition_links() {
    return mediamosa_io::realpath(self::get_uri_transition_links());
  }

  // --------------------------------------------- /temporary related functions.
  /**
   * Return the tmp transcode folder.
   *
   * @return string
   *   The location of the temporary transcode folder; data/transcode.
   *
   * @see get_realpath_temporary()
   */
  public static function get_uri_temporary() {
    return 'mediamosa-storage-temporary://';
  }

  /**
   * Return the translated tmp transcode folder.
   *
   * @return string
   *   The location of the temporary transcode folder; data/transcode.
   */
  public static function get_realpath_temporary() {
    return mediamosa_io::realpath(self::get_uri_temporary());
  }

  /**
   * Returns the full path and filename of the transcode file of
   * given filename.
   *
   * @param string $filename
   *   The filename to use.
   *
   * @return string
   *   The location of the trancode folder with filename.
   */
  public static function get_uri_temporary_file($filename) {
    return self::trim_uri(self::get_uri_temporary()) . mediamosa_io_streamwrapper::create_mediafile_path($filename);
  }

  /**
   * Returns the translated full path and filename of the transcode file of
   * given filename.
   *
   * @param string $filename
   *   The filename to use.
   *
   * @return string
   *   The location of the trancode folder with filename.
   */
  public static function get_realpath_temporary_file($filename) {
    return mediamosa_io::realpath(self::get_uri_temporary_file($filename));
  }

  /**
   * Returns the full path and filename of the status file of given filename.
   *
   * @param string $filename
   *   The status file.
   *
   * @return
   *   The uri to the status file.
   */
  public static function get_uri_status_file($filename) {
    return self::get_uri_temporary_file($filename . '.status');
  }

  /**
   * Returns the full path and filename of the status file of
   * given filename.
   *
   * @param $filename
   *   The status file.
   *
   * @return
   *   The uri to the status file.
   */
  public static function get_realpath_status_file($filename) {
    return mediamosa_io::realpath(self::get_uri_status_file($filename));
  }

  /**
   * Returns the full path and filename of the scene file of given job.
   *
   * @param $job_id
   *   The ID of the job.
   *
   * @return
   *   The URI to the scene file.
   */
  public static function get_uri_scene_file($job_id) {
    return 'mediamosa-storage://data/transcode/' . $job_id . '_scene.txt';
  }

  // ------------------------------------------------- /media related functions.
  /**
   * Return the uri of the media directory.
   *
   * @return string
   *   The uri.
   */
  public static function get_uri_media() {
    return 'mediamosa-storage-media://';
  }

  /**
   * Return the path of the media directory.
   *
   * @return string
   *   The path.
   */
  public static function get_realpath_media() {
    return mediamosa_io::realpath(self::get_uri_media());
  }

  /**
   * Return the uri of the media directory.
   *
   * @return string
   *   The uri.
   */
  public static function get_uri_media_ticket() {
    return 'mediamosa-storage-media://ticket';
  }

  /**
   * Return the path of the media directory.
   *
   * @return string
   *   The path.
   */
  public static function get_realpath_media_ticket() {
    return mediamosa_io::realpath(self::get_uri_media_ticket());
  }

  /**
   * Return the path, usable for URLs. Not for URI usage.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $mediafile_id
   *   (optional) The mediafile ID.
   * @param string $filename
   *   (optional) The filename of the file.
   *
   * @return string
   *   The path.
   */
  public static function get_path_media_permanent_file($app_id, $mediafile_id = '', $filename = '') {
    return $app_id . '/' . (!empty($mediafile_id) ? $mediafile_id[0] . '/' . $mediafile_id . '/' . $filename : '');
  }

  /**
   * Return the realpath of a permanent symlink in the /media directory.
   *
   * The path returned here is an symbolic link to the actual mediafile. The
   * link returned is permanent and is, unlike ticket links, not cleaned up
   * after an amount of time. The permanent link can only exist when the
   * mediafile is not protected by ACL.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $mediafile_id
   *   (optional) The mediafile ID.
   * @param string $filename
   *   (optional) The filename of the file.
   *
   * @return string
   *   The uri.
   */
  public static function get_realpath_media_permanent_file($app_id, $mediafile_id = '', $filename = '') {
    // Return the realpath.
    return self::get_realpath_media() . '/' . self::get_path_media_permanent_file($app_id, $mediafile_id, $filename);
  }

  /**
   * Return the path usable for URLs. Not for URIs.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $ticket_id
   *   The mediafile ID.
   * @param string $filename
   *   The filename of the file.
   * @param string $ticket_type.
   *   (optional) The type of the ticket is used to identify the type of the
   *   link. Is optional in case when $filename already contains the type.
   *
   * @return string
   *   The path.
   */
  public static function get_path_media_ticket_file($app_id, $ticket_id, $filename, $ticket_type = '') {
    // Return the path.
    return 'ticket/' . (empty($ticket_type) ? '' : (strtolower($ticket_type) . '/')) . $app_id . '/' . $ticket_id[0] . '/' . $ticket_id . '/' . $filename;
  }

  /**
   * Return the uri of a temporary symlink in the /media directory.
   *
   * The path returned here is an symbolic link to the actual mediafile. The
   * link returned is temporary and is cleaned up after an amount of time.
   * Ticket based symlinks are used when the file is protected by ACL and access
   * is granted to the user. The ticket ID is random string that only the
   * authenticated party knows.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $ticket_id
   *   The mediafile ID.
   * @param string $filename
   *   The filename of the file.
   * @param string $ticket_type.
   *   (optional) The type of the ticket is used to identify the type of the
   *   link. Is optional in case when $filename already contains the type.
   *
   * @return string
   *   The uri.
   */
  public static function get_uri_media_ticket_file($app_id, $ticket_id, $filename, $ticket_type = '') {
    // Return the uri.
    return self::get_uri_media() . self::get_path_media_ticket_file($app_id, $ticket_id, $filename, $ticket_type);
  }

  /**
   * Return the realpath of a temporary symlink in the /media directory.
   *
   * The path returned here is an symbolic link to the actual mediafile. The
   * link returned is temporary and is cleaned up after an amount of time.
   * Ticket based symlinks are used when the file is protected by ACL and access
   * is granted to the user. The ticket ID is random string that only the
   * authenticated party knows.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $ticket_id
   *   The mediafile ID.
   * @param string $filename
   *   The filename of the file.
   * @param string $ticket_type.
   *   (optional) The type of the ticket is used to identify the type of the
   *   link. Is optional in case when $filename already contains the type.
   *
   * @return string
   *   The uri.
   */
  public static function get_realpath_media_ticket_file($app_id, $ticket_id, $filename, $ticket_type = '') {
    // Return the uri.
    return mediamosa_io::realpath(self::get_uri_media_ticket_file($app_id, $ticket_id, $filename, $ticket_type));
  }

  // ---------------------------------------------------- /media/object related.
  /**
   * Return uri for object files.
   *
   * This location is used for storage and downloading of extra object files,
   * f.e. asx text/html files.
   *
   * @param integer $app_id
   *   (optional) The application ID.
   * @param string $filename
   *   The filename (optional but required when app_id is provided). This can be
   *   f.e. the ticket ID or mediafile ID with file extension.
   *
   * @return
   *   The path to ticket location
   */
  protected static function get_uri_object_file($app_id, $ticket_id, $filename) {
    return 'mediamosa-storage-media://object/' . $app_id . mediamosa_io_streamwrapper::create_mediafile_path($ticket_id) . '/' . $filename;
  }

  /**
   * Return file location for object files.
   *
   * This location is used for storage and downloading of extra object files,
   * f.e. asx text/html files.
   *
   * @param integer $app_id
   *   (optional) The application ID.
   * @param string $filename
   *   The filename (optional but required when app_id is provided). This can be
   *   f.e. the ticket ID or mediafile ID with file extension.
   *
   * @return
   *   The path to ticket location
   */
  public static function get_realpath_object_file($app_id, $ticket_id, $filename) {
    return mediamosa_io::realpath(self::get_uri_object_file($app_id, $ticket_id, $filename));
  }

  /**
   * Return the path to object file (e.g. asx) usable for URLs, not for URIs.
   *
   * @param integer $app_id
   *   The application ID.
   * @param string $ticket_id
   *   The mediafile ID.
   * @param string $filename
   *   The filename of the file.
   *
   * @return string
   *   The path.
   */
  public static function get_path_media_object_file($app_id, $ticket_id, $filename) {
    // Return the path.
    return 'object/' . $app_id . '/' . $ticket_id[0] . '/' . $ticket_id . '/' . $filename;
  }

  // --------------------------------------------------- /ftp related functions.
  /**
   * Get the ftp uri.
   *
   * @return string
   *   The FTP location in uri.
   */
  public static function get_realpath_ftp() {
    return mediamosa_io::realpath('mediamosa-storage://ftp');
  }

  /**
   * Get the SAN/NAS history path.
   *
   * @return string
   *   The FTP history location in uri.
   */
  public static function get_realpath_ftp_history() {
    return mediamosa_io::realpath('mediamosa-storage://ftp/history/');
  }

  /**
   * Get the history file location uri.
   *
   * @return string
   *   The history location uri.
   */
  public static function get_realpath_ftp_history_job() {
    return mediamosa_io::realpath('mediamosa-storage://ftp/history/current.txt');
  }

  /**
   * Returns the contents for the .htaccess file under [mountpoint]/media.
   *
   * @return string
   *   The contents for the .htaccess file for [mount_pount]/media/.htaccess.
   */
  public static function file_get_media_htaccess_contents() {
    $subdir = trim(url(), '/');
    $ht_access = file_get_contents(drupal_get_path('module', 'mediamosa') . '/lib/misc/media_htaccess_contents.txt');
    return strtr($ht_access, array('{SUBDIR}' => $subdir == '' ? '' : '/' . $subdir));
  }

  /**
   * Creates an .htaccess file in the given directory using the given contents.
   *
   * @param string $directory
   *   The destination directory.
   * @param string $contents
   *   The contents to write.
   *
   * @return boolean
   *   Returns either TRUE (success) or FALSE (failure).
   */
  public static function file_create_htaccess($directory, $contents) {
    if (file_uri_scheme($directory)) {
      $directory = file_stream_wrapper_uri_normalize($directory);
    }
    else {
      $directory = rtrim($directory, '/\\');
    }
    $htaccess_path =  $directory . '/.htaccess';

    // If file already exists, then bail out.
    if (mediamosa_io::file_exists($htaccess_path)) {
      return TRUE;
    }

    // Write the .htaccess file.
    if (file_put_contents($htaccess_path, $contents)) {
      drupal_chmod($htaccess_path, 0444);
      return TRUE;
    }

    // Make log entry about error.
    $variables = array('%directory' => $directory, '@path' => $htaccess_path, '!htaccess' => '<br />' . nl2br(check_plain($contents)));
    mediamosa_watchdog::log("Warning: Couldn't write .htaccess file in location @path. Please create a .htaccess file in your %directory directory which contains the following lines: <code>!htaccess</code>", $variables, WATCHDOG_ERROR);
    return FALSE;
  }

  /**
   * Trims the uri correctly without messing up schema.
   *
   * @param string $uri
   *   The uri to trim.
   * @param string $end
   *   By default will add '/' at the end.
   *
   * @return string
   *   The trimed uri.
   */
  public static function trim_uri($uri, $add = '/') {
    return drupal_substr($uri, -3) != '://' ? rtrim($uri, '/') . $add : $uri;
  }

  /**
   * Update the mountpoint to the new location.
   *
   * @param array $mediafile
   *
   * @param string $sannas_mount_point
   */
  public static function mediafile_update_mointpoint($mediafile, $sannas_mount_point) {
    if (is_string($mediafile)) {
      // Provided ID? Then get array from db.
      $mediafile = mediamosa_asset_mediafile::must_exists($mediafile);
    }

    $fields = array(
      mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT => $sannas_mount_point,
    );

    $app_id = $mediafile[mediamosa_asset_mediafile_db::APP_ID];
    $mediafile_id = $mediafile[mediamosa_asset_mediafile_db::ID];
    $user_id = $mediafile[mediamosa_asset_mediafile_db::OWNER_ID];

    // Update the mediafile.
    mediamosa_asset_mediafile::update($app_id, $mediafile_id, $user_id, $fields);
  }

  /**
   * Copy mediafile to any location.
   *
   * @param string $source_uri
   *   The uri of the source file.
   * @param string $destination_uri
   *   The uri of the target file. Must include the filename.
   *
   * @throws mediamosa_exception_error
   */
  public static function mediafile_copy($source_uri, $destination_uri) {
    // This could happen.
    if ($source_uri == $destination_uri) {
      mediamosa_watchdog::log('Not coping mediafile; source and target are the same.');
      return;
    }

    // Find the stream wrapper.
    $mediamosa_io_streamwrapper_source = mediamosa_io::require_stream_wrapper_instance_by_uri($source_uri);

    // Find the stream wrapper destination.
    $mediamosa_io_streamwrapper_destination = mediamosa_io::require_stream_wrapper_instance_by_uri($destination_uri);

    // Now copy.
    $mediamosa_io_streamwrapper_destination->mediafile_copy($mediamosa_io_streamwrapper_source);

    // Copied, return.
  }

  /**
   * Move mediafile from one storage location to another.
   *
   * Moving a mediafile from its current location to another comes in 4 flavors;
   * 1. Moving from local to local storage (move file).
   * 2. Moving from local to external storage (upload file).
   * 3. Moving from external to local storage (download file).
   * 4. Moving from external to external storage (move file).
   *
   * @param array $mediafile
   *   The mediafile to move.
   * @param integer $storage_profile_id
   *   The destination location.
   * @param string $path
   *   The destination path.
   *
   * @throws mediamosa_exception_program_error
   * @throws mediamosa_exception_error
   */
  public static function mediafile_move(array $mediafile, $storage_profile_id, $path) {
    // Source uri.
    $source_uri = mediamosa_storage::get_uri_mediafile($mediafile);

    // Get the storage profile.
    $storage_profile = mediamosa_storage_profile::must_exists($storage_profile_id);

    // Is source mediafile a still?
    $is_still = mediamosa_asset_mediafile::is_still($mediafile);

    // Build the destination uri.
    $mount_point_uri = mediamosa_storage::get_mount_point_uri_by_storage_profile($storage_profile, $path, $is_still);

    // Create tmp wrapper using the mount pount uri (which is not complete).
    $wrapper = mediamosa_io::require_stream_wrapper_instance_by_uri($mount_point_uri);

    // Only our class has create_storage_uri.
    $destination_uri = $wrapper->create_storage_uri($mediafile);

    // Update with complete uri.
    $wrapper->setUri($destination_uri);

    // Copy file to destination.
    self::mediafile_copy($source_uri, $destination_uri);

    // Done, update the new location.
    self::mediafile_update_mointpoint($mediafile, $wrapper->get_uri_mount_point());

    // Store md5 metadata.
    mediamosa_asset_mediafile_metadata::create_mediafile_metadata_char($mediafile[mediamosa_asset_mediafile_db::ID], $wrapper->getMD5(), mediamosa_asset_mediafile_metadata::MD5);

    // Remove old file. If the file is in use somewhere else, it will fail but
    // is not fatal.
    mediamosa_io::unlink($source_uri);
  }

  /**
   * Collect information about current mediafile locations.
   *
   * @param integer $app_id
   *   The application ID.
   * @param boolean $is_still
   *   Select either stills or mediafiles.
   *
   * @return array
   *   An associative array storage location.
   */
  public static function app_collect_storage_locations($app_id, $is_still = FALSE) {

    // Collect the storage locations.
    $query = mediamosa_db::db_select(mediamosa_asset_mediafile_db::TABLE_NAME, 'mf');
    $query->addExpression('COUNT(' . mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT . ')', 'count');
    $query->addField('mf', mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT);
    $query->addField('mf', mediamosa_asset_mediafile_db::IS_STILL);
    $query->condition(mediamosa_asset_mediafile_db::APP_ID, $app_id);
    $query->condition(mediamosa_asset_mediafile_db::IS_STILL, $is_still ? mediamosa_asset_mediafile_db::IS_STILL_TRUE : mediamosa_asset_mediafile_db::IS_STILL_FALSE);
    $query->groupBy(mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT);
    $result = $query->execute();

    // Get the storage profile app for this client application.
    $storage_profile_app = mediamosa_storage_profile_app::get_app_storage_profile($app_id);

    // Build up the mount point uri.
    $app_uri = self::get_mount_point_uri_by_storage_profile_app($storage_profile_app, $is_still);
    $app_scheme = mediamosa_io::get_uri_scheme($app_uri);

    // In case of unknown.
    $unknown = array(
      'name' => t('Unknown storage profile'),
      'description' => t("Storage profile '@uri' not registered."),
    );

    // Get the wrappers.
    $wrappers = file_get_stream_wrappers();

    $storage = array();
    $total = 0;
    $total_to_move = 0;
    foreach ($result as $mount_point) {
      // If for some reason the mountpoint is empty, assume default.
      if (empty($mount_point[mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT])) {
        $mount_point[mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT] = mediamosa_storage::get_uri_data();
      }

      // Get uri of the grouped mediafiles.
      $uri = $mount_point[mediamosa_asset_mediafile_db::SANNAS_MOUNT_POINT];
      $scheme = mediamosa_io::get_uri_scheme($uri);

      // Create wrapper.
      $wrapper = isset($wrappers[$scheme]) ? $wrappers[$scheme] : $unknown;
      $wrapper_instance = mediamosa_io::find_stream_wrapper_instance_by_uri($uri);

      // Check for storage profile.
      if ($wrapper_instance) {
        $storage_profile_id = $wrapper_instance->get_storage_profile_id();
        try {
          mediamosa_storage_profile::must_exists($storage_profile_id);
        }
        catch (mediamosa_exception $e) {
          $wrapper_instance = FALSE;
        }
      }

      $storage[$uri]['wrapper_instance'] = $wrapper_instance;

      $is_local = $wrapper_instance ? (method_exists($wrapper_instance, 'is_local') ? $wrapper_instance->is_local() : TRUE) : FALSE;
      $storage[$uri]['is_local'] = $is_local;
      $path = $is_local ? $wrapper_instance->realpath() : '';
      $storage[$uri]['path'] = $path;

      if (empty($storage[$uri]['count'])) {
        $storage[$uri]['count'] = $mount_point['count'];
      }
      else {
        $storage[$uri]['count'] += $mount_point['count'];
      }

      $total += $mount_point['count'];

      $storage[$uri]['name'] = $wrapper['name'];
      $storage[$uri]['description'] = strtr($wrapper['description'], array('@uri' => $uri));

      $storage[$uri]['movable'] = FALSE;
      if ($app_uri != $uri) {
        $storage[$uri]['movable'] = TRUE;
        $total_to_move += $mount_point['count'];
      }
    }

    return array('data' => $storage, 'total' => $total, 'total_to_move' => $total_to_move, 'app_scheme' => $app_scheme, 'app_uri' => $app_uri);
  }
}
