<?php

/**
 * Implements hook_user_delete().
 */
function omni_eve_api_user_delete($account)
{
  db_delete('oea_characters') -> condition('drupalid', $account -> uid) -> execute();
  db_delete('oea_api_keys') -> condition('drupalid', $account -> uid) -> execute();
}

/**
 * Override template_preprocess_user_picture() to display user pictures with
 * EVE Online integration.
 *
 * Example URL's
 * http://image.eveonline.com/Character/1643072492_128.jpg
 * http://image.eveonline.com/Character/1_128.jpg
 *
 * @see template_preprocess_user_picture()
 * @see _gravatar_load_account()
 * @see _gravatar_get_account_user_picture()
 */
function omni_eve_api_preprocess_user_picture(&$variables)
{
  $variables['user_picture'] = '';

  if (variable_get('user_pictures', 0))
  {
    // Load the full user object since it is not provided with nodes, comments,
    // or views displays.
    $account = _omni_eve_api_load_account($variables['account']);
    $filepath = _omni_eve_api_get_account_user_picture($account);

    if (!empty($filepath))
    {
      $alt = t("@user's picture", array('@user' => format_username($account)));
      if (module_exists('image') && file_valid_uri($filepath) && $style = variable_get('user_picture_style', ''))
      {
        $variables['user_picture'] = theme('image_style', array(
          'style_name' => $style,
          'path' => $filepath,
          'alt' => $alt,
          'title' => $alt
        ));
      }
      else
      {
        $variables['user_picture'] = theme('image', array(
          'path' => $filepath,
          'alt' => $alt,
          'title' => $alt
        ));
      }
      if ($account -> uid && user_access('access user profiles'))
      {
        // Create link to the user's profile.
        $attributes = array('title' => t('View user profile.'));
        $variables['user_picture'] = l($variables['user_picture'], 'user/' . $account -> uid, array(
          'attributes' => $attributes,
          'html' => TRUE
        ));
      }
      elseif (!empty($account -> homepage))
      {
        // If user is anonymous, create link to the commenter's homepage.
        $attributes = array(
          'title' => t('View user website.'),
          'rel' => 'external nofollow',
        );
        $variables['user_picture'] = l($variables['user_picture'], $account -> homepage, array(
          'attributes' => $attributes,
          'html' => TRUE
        ));
      }
    }
  }
}

/**
 * Original function from"Gravatar v1.1" by Narno, Git Migration, Dave Reid, podarok, and macedigital
 * Unmodified function.
 */
function _omni_eve_api_load_account($account)
{
  // If this is a node or comment object, load the user object.
  if (!empty($account -> nid) || !empty($account -> cid) || empty($account -> roles))
  {
    $original_values = $account;

    // If a comment is being edited and previewed, the $account->uid is NULL.
    // @todo Remove when http://drupal.org/node/334826 is fixed in 6.x.
    if (!isset($account -> uid))
    {
      $account -> uid = 0;
    }

    $account = $account -> uid ? user_load($account -> uid) : drupal_anonymous_user();

    // Load mail/homepage variable from an anonymous comment.
    if (!$account -> uid)
    {
      $values = array_fill_keys(array(
        'name',
        'mail',
        'homepage',
        'hostname'
      ), '');
      foreach ($values as $value => $default_value)
      {
        if (empty($account -> $value))
        {
          $account -> $value = !empty($original_values -> $value) ? $original_values -> $value : $default_value;
        }
      }
    }
  }

  if (isset($account -> picture) && is_numeric($account -> picture))
  {
    $account -> picture = file_load($account -> picture);
  }

  return $account;
}

/**
 * Original function from"Gravatar v1.1" by Narno, Git Migration, Dave Reid, podarok, and macedigital
 *
 * Modified for use with "Omni EVE API"
 *
 * Decide which user picture should be displayed for a user account.
 *
 * @param $account
 *   A user object.
 * @return
 *   A string with the path to the user's picture.
 */
function _omni_eve_api_get_account_user_picture($account)
{
  $user = user_load($account -> uid);

  if (!empty($account -> picture -> uri))
  {
    // If the user has an uploaded picture, use it first.
    return $account -> picture -> uri;
  }
  elseif (empty($user -> characterid))
  {
    return omni_eve_api_get_eve_avatar('1');
  }
  else
  {
    return omni_eve_api_get_eve_avatar($user -> characterid);
  }
}

/**
 * Generate a EVE Avatar URL.
 *
 * @param $characterid
 *   A string with an EVE character ID.
 * @return
 *   An URL string with the EVE avatar image.
 */
function omni_eve_api_get_eve_avatar($characterid)
{
  global $is_https;

  $eve_avatar = $is_https ? variable_get('eve_avatar_https') : variable_get('eve_avatar_http');
  $eve_avatar .= $characterid . '_' . variable_get('eve_user_picture_dimensions') . '.jpg';

  return url($eve_avatar);
}

/**
 * Function that retrieves all API's associated with a $uid
 */
function omni_eve_api_list_api($uid)
{
  $api_list = array();
  $api_list['full'] = $api_list['simple'] = array();

  $result_a = db_select('oea_api_keys', 'a') -> fields('a', array(
    'apiid',
    'keyid',
    'vcode',
    'errorid',
    'updated',
    'created',
  )) -> condition('drupalid', $uid, '=') -> execute();

  if ($result_a -> rowCount())
  {
    foreach ($result_a->fetchAll() as $row_a)
    {
      $api_list['full'][$row_a -> apiid] = array(
        'apiid' => $row_a -> apiid,
        'keyid' => $row_a -> keyid,
        'vcode' => $row_a -> vcode,
        'errorid' => $row_a -> errorid,
        'updated' => $row_a -> updated,
        'created' => $row_a -> created,
      );

      $result_b = db_select('oea_characters', 'c') -> fields('c', array(
        'characterid',
        'charactername',
        'corporationid',
        'corporationname',
        'corporationticker',
        'allianceid',
        'alliancename',
        'allianceticker',
      )) -> condition('drupalid', $uid, '=') -> condition('apiid', $row_a -> apiid, '=') -> execute();

      if ($result_b -> rowCount())
      {
        foreach ($result_b->fetchAll() as $row_b)
        {
          $api_list['full'][$row_a -> apiid]['characters'][$row_b -> characterid] = array(
            'characterid' => $row_b -> characterid,
            'charactername' => $row_b -> charactername,
            'corporationid' => $row_b -> corporationid,
            'corporationname' => $row_b -> corporationname,
            'corporationticker' => $row_b -> corporationticker,
            'allianceid' => $row_b -> allianceid,
            'alliancename' => $row_b -> alliancename,
            'allianceticker' => $row_b -> allianceticker,
          );
          $api_list['simple']['characters'][$row_b -> characterid] = array(
            'characterID' => $row_b -> characterid,
            'characterName' => $row_b -> charactername,
            'corporationID' => $row_b -> corporationid,
            'corporationName' => $row_b -> corporationname,
            'corporationTicker' => $row_b -> corporationticker,
            'allianceID' => $row_b -> allianceid,
            'allianceName' => $row_b -> alliancename,
            'allianceTicker' => $row_b -> allianceticker,
          );
        }
      }
    }
  }

  return $api_list;
}

/**
 * Add function to add a new API
 */
function omni_eve_api_user_add_api_form($form, &$form_state, $account)
{
  global $user;

  if ($account -> uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $uid = $account -> uid;

  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Verify Blue Status'),
    '#description' => t('Please enter your EVE API in order to add it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['enter_api']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask') . '">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['enter_api']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask') . '">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
  );

  $form['enter_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add API Key'),
  );

  $form['#validate'] = array('omni_eve_api_user_add_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_add_api_form_submit');

  return $form;
}

/**
 * Custom validation function for adding additional API's for verification
 */
function omni_eve_api_user_add_api_form_validate($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (!$characters['expires'] && $characters['type'] == 'Account' && $characters['accessMask'] == variable_get('omni_eve_api_access_mask'))
  {
    if ($chars = omni_eve_api_characters_exist($characters))
    {
      form_set_error('keyid', 'Characters on this key have already been registered. Characters registered: ' . implode(", ", $chars));
      form_set_error('vcode');
    }
  }
  else
  {
    form_set_error('keyid', 'Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.');
    form_set_error('vcode');
  }
}

/**
 * Custom submit function for adding additional API's for submission
 */
function omni_eve_api_user_add_api_form_submit($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  $created = date('Y-m-d H:i:s', time());

  $new_api_key_id = db_insert('oea_api_keys') -> fields(array(
    'drupalid' => $uid,
    'keyid' => $keyid,
    'vcode' => $vcode,
    'updated' => $created,
    'created' => $created,
  )) -> execute();

  foreach ($characters['characters'] as $character)
  {
    db_merge('oea_characters') -> key(array('characterid' => $character['corporationID'])) -> fields(array(
      'characterid' => $character['characterID'],
      'drupalid' => $uid,
      'apiid' => $new_api_key_id,
      'charactername' => $character['characterName'],
      'corporationid' => $character['corporationID'],
      'corporationname' => $character['corporationName'],
      'corporationticker' => $character['corporationTicker'],
      'allianceid' => $character['allianceID'],
      'alliancename' => $character['allianceName'],
      'allianceticker' => $character['allianceTicker'],
    )) -> execute();
  }

  drupal_set_message(t('API Key successfully added!'));
}

/**
 * Add function to list valid characters the user can select from for a new main character
 */
function omni_eve_api_user_select_main_character_form($form, &$form_state, $account)
{
  global $user;

  if ($account -> uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $uid = $account -> uid;
  $uid_api_list = omni_eve_api_list_api($uid);

  $form['main_character'] = array(
    '#type' => 'fieldset',
    '#title' => t('Valid Characters'),
    '#weight' => 0,
  );

  $form['main_character']['select_charactername'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => $account -> charactername,
    '#options' => omni_eve_api_valid_characters($uid_api_list['simple']),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );

  $form['main_character']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['#validate'] = array('omni_eve_api_user_select_main_character_form_validate');
  $form['#submit'] = array('omni_eve_api_user_select_main_character_form_submit');

  return $form;
}

/**
 * Custom validation function for selecting a new main character for verification
 */
function omni_eve_api_user_select_main_character_form_validate($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }
}

/**
 * Custom submit function for selecting a new main character for submission
 */
function omni_eve_api_user_select_main_character_form_submit($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $account = user_load($uid);

  if ($account -> charactername != $form_state['values']['select_charactername'] && !empty($form_state['values']['select_charactername']))
  {
    $old_charactername = $account -> charactername;
    $old_character_data = omni_eve_api_get_character_info($old_charactername);
    $charactername = $form_state['values']['select_charactername'];
    $character_data = omni_eve_api_get_character_info($charactername);

    $old_rid_2 = NULL;

    if ($corporation_role = user_role_load_by_name($old_character_data['corporationName']))
    {
      $old_rid = $corporation_role -> rid;

      if ($alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role')))
      {
        $old_rid_2 = $alliance_role -> rid;
      }
    }
    elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role')))
    {
      $old_rid = $blue_role -> rid;
    }

    user_multiple_role_edit(array($uid), 'remove_role', $old_rid);

    if (isset($old_rid_2))
    {
      user_multiple_role_edit(array($uid), 'remove_role', $old_rid_2);
    }

    db_update('users') -> fields(array('characterid' => $character_data['characterID'], )) -> condition('uid ', $uid, '=') -> execute();
    db_update('users') -> fields(array('charactername' => $character_data['characterName'], )) -> condition('uid ', $uid, '=') -> execute();

    $new_rid_2 = NULL;

    if ($corporation_role = user_role_load_by_name($character_data['corporationName']))
    {
      $new_rid = $corporation_role -> rid;

      if ($alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role')))
      {
        $new_rid_2 = $alliance_role -> rid;
      }
    }
    elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role')))
    {
      $new_rid = $blue_role -> rid;
    }
    else
    {
      $default_role = user_role_load_by_name('authenticated user');
      $new_rid = $default_role -> rid;
    }

    user_multiple_role_edit(array($uid), 'add_role', $new_rid);

    if (isset($new_rid_2))
    {
      user_multiple_role_edit(array($uid), 'add_role', $new_rid_2);
    }

    drupal_set_message(t('New main character selected!'));
  }
}

/**
 * Add function to show all API's the user currently has and delete them via selection
 */
function omni_eve_api_user_list_api_form($form, &$form_state, $account)
{
  global $user;

  if ($account -> uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $uid = $account -> uid;
  $uid_api_list = omni_eve_api_list_api($uid);

  $header = array(
    'edit' => array(
      'data' => t('Edit'),
      'field' => 'n.edit'
    ),
    'keyid' => array(
      'data' => t('API Key ID'),
      'field' => 'n.keyid'
    ),
    'vcode' => array(
      'data' => t('API Verification Code'),
      'field' => 'n.vcode'
    ),
    'characters' => array(
      'data' => t('Characters'),
      'field' => 'n.characters'
    ),
    'updated' => array(
      'data' => t('Last Updated'),
      'field' => 'n.updated'
    ),
    'created' => array(
      'data' => t('Created'),
      'field' => 'n.created'
    ),
    'status' => array(
      'data' => t('Status'),
      'field' => 'n.status'
    ),
  );

  $options = array();

  foreach ($uid_api_list['full'] as $uid_api_single)
  {
    $list_characters = '';

    foreach ($uid_api_single['characters'] as $characters)
    {
      $corp_alliance = ($characters['allianceid'] == 0) ? $characters['corporationname'] : $characters['corporationname'] . '/' . $characters['alliancename'];
      $list_characters .= '[' . $characters['corporationticker'] . '] ' . $characters['charactername'] . ' (' . $corp_alliance . ')';
      $list_characters .= '<br />';
    }

    $options[$uid_api_single['apiid']] = array(
      'edit' => array('data' => array(
          '#type' => 'link',
          '#title' => t('Edit API # @api_key_id', array('@api_key_id' => $uid_api_single['apiid'])),
          '#href' => 'user/' . $uid . '/omni_eve_api/update_api_key/' . $uid_api_single['apiid'],
        ), ),
      'keyid' => $uid_api_single['keyid'],
      'vcode' => $string = (strlen($uid_api_single['vcode']) > 13) ? substr($uid_api_single['vcode'], 0, 10) . '...' : $uid_api_single['vcode'],
      'characters' => $list_characters,
      'updated' => format_date(strtotime($uid_api_single['updated']), 'short'),
      'created' => format_date(strtotime($uid_api_single['created']), 'short'),
      'status' => $uid_api_single['errorid'],
    );
  }

  $form['list_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['list_api']['api_keys'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No API Keys associated with your account.'),
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
  );

  $form['#validate'] = array('omni_eve_api_user_list_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_list_api_form_submit');

  return $form;
}

/**
 * Custom validation function for api's that can be deleted for verification
 */
function omni_eve_api_user_list_api_form_validate($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }
}

/**
 * Custom submit function for api's that can be deleted for submission
 */
function omni_eve_api_user_list_api_form_submit($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  foreach ($form_state['values']['api_keys'] as $api_key_id)
  {
    if ($api_key_id == 0)
    {
      continue;
    }

    db_delete('oea_api_keys') -> condition('apiid', $api_key_id) -> execute();
    db_delete('oea_characters') -> condition('apiid', $api_key_id) -> execute();

    drupal_set_message(t('API # @api_key_id  successfully deleted!', array('@api_key_id' => $api_key_id)));
  }

  $query = db_select('oea_characters', 'c');
  $query -> join('users', 'u', 'c.charactername = u.charactername');
  $query -> fields('c', array(
    'charactername',
    'characterid'
  ));
  $query -> condition('u.uid', $uid);
  $result = $query -> execute();

  if (!$result -> rowCount())
  {
    db_update('users') -> fields(array(
      'charactername' => '',
      'characterid' => 0,
    )) -> condition('uid ', $uid, '=') -> execute();
    $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
    drupal_set_message(t('Please select a new Main Character'));
  }
}

/**
 * Add function to update an API
 */
function omni_eve_api_user_update_api_form($form, &$form_state, $account, $api_key_id)
{
  global $user;

  if ($account -> uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $uid = $account -> uid;

  $result = db_select('oea_api_keys', 'a') -> fields('a', array(
    'keyid',
    'vcode',
  )) -> condition('apiid ', $api_key_id, '=') -> execute();

  if (!$result -> rowCount())
  {
    drupal_goto('user/' . $uid . '/omni_eve_api');
  }

  $row = $result -> fetchAssoc();

  $form['update_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update API'),
    '#description' => t('Please enter your EVE API in order to update it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['update_api']['api_key_id'] = array(
    '#value' => $api_key_id,
    '#type' => 'hidden',
  );

  $form['update_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['update_api']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask') . '">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#default_value' => isset($form_state['values']['keyid']) ? $form_state['values']['keyid'] : $row['keyid'],
    '#weight' => 0,
  );

  $form['update_api']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask') . '">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#default_value' => isset($form_state['values']['vcode']) ? $form_state['values']['vcode'] : $row['vcode'],
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update API Key'),
  );

  $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api';
  $form['#validate'] = array('omni_eve_api_user_update_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_update_api_form_submit');

  return $form;
}

/**
 * Custom validation function for updating an API's for verification
 */
function omni_eve_api_user_update_api_form_validate($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];
  $api_key_id = $form_state['values']['api_key_id'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if ($characters['expires'] || $characters['type'] != 'Account' || $characters['accessMask'] != variable_get('omni_eve_api_access_mask'))
  {
    form_set_error('keyid', 'Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.');
    form_set_error('vcode');
  }
}

/**
 * Custom submit function for updating an API for submission
 */
function omni_eve_api_user_update_api_form_submit($form, &$form_state)
{
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user -> uid && !user_access('omni eve api moderate'))
  {
    drupal_goto('<front>');
  }

  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];
  $api_key_id = $form_state['values']['api_key_id'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  $updated = date('Y-m-d H:i:s', time());

  db_delete('oea_characters') -> condition('apiid', $api_key_id) -> execute();

  db_merge('oea_api_keys') -> key(array('apiid' => $api_key_id)) -> fields(array(
    'apiid' => $api_key_id,
    'drupalid' => $uid,
    'keyid' => $keyid,
    'vcode' => $vcode,
    'updated' => $updated,
  )) -> execute();

  foreach ($characters['characters'] as $character)
  {
    db_merge('oea_characters') -> key(array('characterid' => $character['corporationID'])) -> fields(array(
      'characterid' => $character['characterID'],
      'drupalid' => $uid,
      'apiid' => $api_key_id,
      'charactername' => $character['characterName'],
      'corporationid' => $character['corporationID'],
      'corporationname' => $character['corporationName'],
      'corporationticker' => $character['corporationTicker'],
      'allianceid' => $character['allianceID'],
      'alliancename' => $character['allianceName'],
      'allianceticker' => $character['allianceTicker'],
    )) -> execute();
  }

  $query = db_select('oea_characters', 'c');
  $query -> join('users', 'u', 'c.charactername = u.charactername');
  $query -> fields('c', array(
    'charactername',
    'characterid'
  ));
  $query -> condition('u.uid', $uid);
  $result = $query -> execute();

  if (!$result -> rowCount())
  {
    db_update('users') -> fields(array(
      'charactername' => '',
      'characterid' => 0,
    )) -> condition('uid ', $uid, '=') -> execute();
    $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
    drupal_set_message(t('Please select a new Main Character'));
  }

  drupal_set_message(t('API Key successfully updated!'));
}