<?php
/**
 * Handles admin_menu_by_role settings form display
 *
 * @return
 *  A form
 *
 * @see admin_menu_by_role_settings_form_submit()
 * @see theme_admin_menu_by_role_settings_form()
 * @ingroup forms
 */
function admin_menu_by_role_settings_form(&$form_state) {


  // convert toolbar names as numeric
  // avoid 'Illegal choice...' problem with form validation
  $result = db_query("SELECT * FROM {menu_custom} ORDER BY title");
  $menu_options = array();
  $id = 0;
  while ($menu = db_fetch_array($result)) {
  	$admin_menu_id[$menu['menu_name']] = $id;
  	$admin_menu_options[$id++] = $menu['title'];
  }

  $max_weight = db_result(db_query("SELECT MIN(weight) FROM {admin_menu_role}"));
  if ( FALSE === $max_weight ) {
    // if first item
    $max_weight = 0;
  }
  else {
    $max_weight++;
  }

  $weight = array();
  $admin_menu_role = array();
  $roles = user_roles(TRUE, 'access administration menu');

  $key = 0;

  // set defaults, and set role title
  foreach ($roles as $rid => $role) {
    $result = db_query('SELECT * FROM {admin_menu_role} WHERE rid=%d', $rid);
    $row = db_fetch_array($result);
    if ( FALSE === $row ) {
      $admin_menu_role[$key] = array(
        'rid'     => $rid,
        'name'    => 0,
        'weight'  => ++$max_weight,
        'title'   => $role
      );
    }
    else {
      $admin_menu_role[$key] = array(
        'rid'     => (int)$row['rid'],
        'name'    => $admin_menu_id[$row['name']],
        'weight'  => (int)$row['weight'],
        'title'   => $role
      );
    }
    $weight[$key] = $admin_menu_role[$key]['weight'];
    $key++;
  }

  // sort by weight
  array_multisort($weight, SORT_ASC, $admin_menu_role);
  $form = array();

  // save some data, no need to fetch them again
  $form['#admin_menu_id'] = $admin_menu_id;

  $form['roles']['#tree'] = TRUE;
  $delta = count($roles);

  foreach ($admin_menu_role as $item) {
    $rid = $item['rid'];
    $form['roles'][$rid]['title'] = array(
      '#type' => 'markup',
      '#value' => check_plain($item['title']),
    );
    $form['roles'][$rid]['menu'] = array(
      '#type' => 'select',
      '#default_value' => $item['name'],
      '#options' => $admin_menu_options
    );
    $form['roles'][$rid]['weight'] = array(
      '#type'           => 'weight',
      '#delta'          => $delta,
      '#default_value'  => $item['weight'],
    );
    $form['roles'][$rid]['id'] = array(
      '#type' => 'hidden',
      '#value' => $rid,
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration')
  );

  return $form;
}

/**
 * Submit handler for toolbar_settings_form()
 *
 * @see toolbar_shortcuts_settings_form()
 * @ingroup forms
 */
function admin_menu_by_role_settings_form_submit($form, &$form_state) {
  foreach ($form_state['values']['roles'] as $rid => $values) {

    $current = $form['roles'][$rid]['menu']['#default_value'];
    if ( '0' == $current ) {
      $current = NULL;
    }

    $new = $values['menu'];
    if ( '0' == $new ) {
      $new = NULL;
    }
    else {
      $new = array_search($new, $form['#admin_menu_id']);
    }

    if ( is_null($current) ) {
      if ( !is_null($new) ) {
        // if current value is not set (null) and toolbar changed (not null), insert
        db_query("INSERT INTO {admin_menu_role} VALUES(%d, '%s', %d)", $rid, $new, $values['weight']);
      }
    }
    else {
      if ( is_null($new) ) {
        // if current value is set (not null) and new value is disabled(not null) - delete
        db_query("DELETE FROM {admin_menu_role} WHERE rid=%d ", $rid);

      }
      else {
        // if current value is set (not null) and new value is different (not 'current'),
        // but not disabled (not null) - update
        db_query("UPDATE {admin_menu_role} SET name='%s', weight=%d WHERE rid=%d", $new, $values['weight'], $rid);
      }
    }
  }
  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Theme handler for toolbar_settings_form()
 *
 * @see theme_toolbar_settings_form()
 * @ingroup themable
 */
function theme_admin_menu_by_role_settings_form($form) {
  $rows = array();
  foreach (element_children($form['roles']) as $rid) {
    $element = &$form['roles'][$rid];
    $row = array();

    $element['weight']['#attributes']['class'] = 'weight-group';
    $element['itemid']['#attributes']['class'] = 'itemid';
    $row[] = drupal_render($element['title']);
    $row[] = drupal_render($element['menu']);
    $row[] = drupal_render($element['weight']) . drupal_render($element['id']);

    $rows[] = array('data' => $row, 'class' => 'draggable');
  }
  if (empty($rows)) {
    $empty_message = t('No menu found');
    $rows[] = array(array('data' => $empty_message, 'colspan' => '3'));
  }
  $header = array(t('Role'), t('Menu'), t('Weight'));
  drupal_add_tabledrag('admin_menu_by_role_roles_order_table', 'order', 'sibling', 'weight-group');
  $output = theme('table', $header, $rows, array('id' => 'admin_menu_by_role_roles_order_table'));
  $output .= drupal_render($form);

  return $output;
}