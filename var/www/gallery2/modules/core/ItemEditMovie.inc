<?php
/*
 * $RCSfile: ItemEditMovie.inc,v $
 *
 * Gallery - a web based photo album viewer and editor
 * Copyright (C) 2000-2006 Bharat Mediratta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @version $Revision: 1.13 $ $Date: 2006/01/10 04:39:20 $
 * @package GalleryCore
 * @subpackage UserInterface
 * @author Bharat Mediratta <bharat@menalto.com>
 */

/**
 * Plugin for editing Movies
 *
 * @package GalleryCore
 * @subpackage UserInterface
 */
class ItemEditMovie extends ItemEditPlugin {

    /**
     * @see ItemEditPlugin::handleRequest
     */
    function handleRequest($form, &$item, &$preferred) {
	global $gallery;

	$status = null;
	$error = array();

	/* Figure out which command we're taking */
	if (isset($form['action']['save'])) {

	    /* Missing values get set to 0 */
	    if (isset($form['width']) && $form['width'] < 0) {
		$error[] = 'form[error][width][invalid]';
	    } else {
		$form['width'] = (int)$form['width'];
	    }

	    if (isset($form['height']) && $form['height'] < 0) {
		$error[] = 'form[error][height][invalid]';
	    } else {
		$form['height'] = (int)$form['height'];
	    }

	    if (empty($error)) {
		list ($ret, $lock) = GalleryCoreApi::acquireWriteLock($item->getId());
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null, null, null);
		}

		list ($ret, $item) = $item->refresh();
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lock);
		    return array($ret->wrap(__FILE__, __LINE__), null, null, null);
		}

		$item->setWidth($form['width']);
		$item->setHeight($form['height']);
		$item->setSerialNumber($form['serialNumber']);
		$ret = $item->save();
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lock);
		    return array($ret->wrap(__FILE__, __LINE__), null, null, null);
		}

		$ret = GalleryCoreApi::releaseLocks($lock);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null, null, null);
		}

		/* Prepare our status message */
		list ($ret, $module) = GalleryCoreApi::loadPlugin('module', 'core');
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null, null, null);
		}

		$status = $module->translate('Changes saved successfully');
	    }
	}

	return array(null, $error, $status, false);
    }

    /**
     * @see ItemEditPlugin::loadTemplate
     */
    function loadTemplate(&$template, &$form, $item, $thumbnail) {
	global $gallery;

	if ($form['formName'] != 'ItemEditMovie') {
	    /* First time around, load the form with item data */
	    $form['width'] = $item->getWidth();
	    $form['height'] = $item->getHeight();
	    $form['formName'] = 'ItemEditMovie';
	}

	$ItemEditMovie = array();

	$template->setVariable('ItemEditMovie', $ItemEditMovie);
	$template->setVariable('controller', 'core.ItemEditMovie');
	return array(null,
		     'modules/core/templates/ItemEditMovie.tpl', 'modules_core');
    }

    /**
     * @see ItemEditPlugin::isSupported
     */
    function isSupported($item, $thumbnail) {
	return (GalleryUtilities::isA($item, 'GalleryMovieItem'));
    }

    /**
     * @see ItemEditPlugin::getTitle
     */
    function getTitle() {
	list ($ret, $module) = GalleryCoreApi::loadPlugin('module', 'core');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	return array(null, $module->translate('Movie Size'));
    }
}
?>
