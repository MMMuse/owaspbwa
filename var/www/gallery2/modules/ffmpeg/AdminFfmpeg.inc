<?php
/*
 * $RCSfile: AdminFfmpeg.inc,v $
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
 * @version $Revision: 1.24 $ $Date: 2006/02/14 03:57:41 $
 * @package Ffmpeg
 * @author Bharat Mediratta <bharat@menalto.com>
 */

GalleryCoreApi::requireOnce('modules/ffmpeg/classes/FfmpegToolkitHelper.class');

/**
 * @package Ffmpeg
 * @subpackage UserInterface
 */
class AdminFfmpegController extends GalleryController {

    /**
     * @see GalleryController::handleRequest()
     */
    function handleRequest(&$form) {
	global $gallery;

	$ret = GalleryCoreApi::assertUserIsSiteAdministrator();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$status = $error = array();
	if (isset($form['action']['save']) || isset($form['action']['test'])) {

	    if (empty($form['path'])) {
		$error[] = 'form[error][path][missing]';
	    } else {

		/* Verify the path */
		$platform =& $gallery->getPlatform();
		$slash = $platform->getDirectorySeparator();
		if (!$platform->isRestrictedByOpenBaseDir($form['path'])) {
		    if ($platform->file_exists($form['path']) && $platform->is_dir($form['path'])) {
			$form['path'] = trim($form['path']);
			if (!empty($form['path'])) {
			    if ($form['path']{strlen($form['path'])-1} != $slash) {
				$form['path'] .= $slash;
			    }
			}

			/* Try adding on "ffmpeg" */
			$path = $form['path'] . 'ffmpeg';

			if (!$platform->file_exists($path)) {
			    $error[] = 'form[error][path][badPath]';
			} else {
			    /* Got a match */
			    $form['path'] = $path;
			    GalleryUtilities::putRequestVariable('form.path', $path);
			}
		    }

		    if (empty($error)) {
			if ($platform->is_file($form['path'])
				&& !$platform->is_executable($form['path'])) {
			    $error[] = 'form[error][path][notExecutable]';
			}
		    }
		}

		if (empty($error)) {
		    list ($ret, $testResults) =
			FfmpegToolkitHelper::testBinary($form['path']);
		    if ($ret) {
			if ($ret->getErrorCode() & ERROR_BAD_PATH) {
			    $error[] = 'form[error][path][badPath]';
			} else {
			    return array($ret->wrap(__FILE__, __LINE__), null);
			}
		    } else {
			$failCount = 0;
			foreach ($testResults as $testResult) {
			    /* At least one test should work, else this path is not a valid one */
			    if (!$testResult['success']) {
				$failCount++;
			    }
			}

			if ($failCount > 0) {
			    $error[] = 'form[error][path][testError]';
			}
		    }
		}
	    }

	    if (empty($error) && isset($form['action']['save'])) {
		if (isset($form['addWatermark']) && $form['addWatermark']) {
		    $query = '
		    SELECT [GalleryDerivative::id]
		    FROM [GalleryDerivative], [GalleryChildEntity], [GalleryEntity]
		    WHERE [GalleryDerivative::id] = [GalleryChildEntity::id]
		      AND [GalleryChildEntity::parentId] = [GalleryEntity::id]
		      AND [GalleryEntity::entityType] = \'GalleryMovieItem\'';
		    $ret = $this->_processThumbnails($query, '_addWatermark');
		    if ($ret) {
			return array($ret->wrap(__FILE__, __LINE__), null);
		    }
		    $status['added'] = 1;
		} else if (isset($form['removeWatermark']) && $form['removeWatermark']) {
		    $query = '
		    SELECT [GalleryDerivative::id]
		    FROM [GalleryDerivative]
		    WHERE [GalleryDerivative::derivativeOperations]
			  LIKE \'%composite|%filmreel.png%\'';
		    $ret = $this->_processThumbnails($query, '_removeWatermark');
		    if ($ret) {
			return array($ret->wrap(__FILE__, __LINE__), null);
		    }
		    $status['removed'] = 1;
		}

		$ret = GalleryCoreApi::setPluginParameter('module', 'ffmpeg',
							  'path', $form['path']);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}
		$ret = GalleryCoreApi::setPluginParameter('module', 'ffmpeg',
		    'useWatermark', (isset($form['useWatermark']) &&
				     $form['useWatermark']) ? 1 : 0);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		list ($ret, $module) = GalleryCoreApi::loadPlugin('module', 'ffmpeg');
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}
		list ($ret, $isActive) = $module->isActive();
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		$redirect['view'] = 'core.SiteAdmin';
		if ($isActive) {
		    $redirect['subView'] = 'ffmpeg.AdminFfmpeg';
		    $status['saved'] = 1;
		} else {
		    $redirect['subView'] = 'core.AdminModules';
		    $status['configured'] = 'ffmpeg';
		}
	    }
	} else if (isset($form['action']['reset'])) {
	    $redirect['view'] = 'core.SiteAdmin';
	    $redirect['subView'] = 'ffmpeg.AdminFfmpeg';
	} else if (isset($form['action']['cancel'])) {
	    $redirect['view'] = 'core.SiteAdmin';
	    $redirect['subView'] = 'core.AdminModules';
	}

	if (!empty($redirect)) {
	    $results['redirect'] = $redirect;
	} else {
	    $results['delegate']['view'] = 'core.SiteAdmin';
	    $results['delegate']['subView'] = 'ffmpeg.AdminFfmpeg';
	}
	$results['status'] = $status;
	$results['error'] = $error;

	return array(null, $results);
    }

    function _processThumbnails($query, $operation) {
	global $gallery;
	list ($ret, $searchResults) = $gallery->search($query);
	if ($ret) {
	    return $ret->wrap(__FILE__, __LINE__);
	}
	$ids = array();
	while ($result = $searchResults->nextResult()) {
	    $ids[] = $result[0];
	}
	while (!empty($ids)) {
	    $idSet = array_splice($ids, 0, 100);
	    list ($ret, $lockId) = GalleryCoreApi::acquireWriteLock($idSet);
	    if ($ret) {
		return $ret->wrap(__FILE__, __LINE__);
	    }
	    list ($ret, $thumbnails) = GalleryCoreApi::loadEntitiesById($idSet);
	    if ($ret) {
		GalleryCoreApi::releaseLocks($lockId);
		return $ret->wrap(__FILE__, __LINE__);
	    }
	    foreach ($thumbnails as $thumbnail) {
		$this->$operation($thumbnail);
		$ret = $thumbnail->save();
		if ($ret) {
		    GalleryCoreApi::releaseLocks($lockId);
		    return $ret->wrap(__FILE__, __LINE__);
		}
	    }
	    $ret = GalleryCoreApi::releaseLocks($lockId);
	    if ($ret) {
		return $ret->wrap(__FILE__, __LINE__);
	    }
	}
	return null;
    }

    function _addWatermark(&$thumbnail) {
	static $op = 'composite|plugins_data/modules/ffmpeg/filmreel.png,image/png,12,399';
	$operations = $thumbnail->getDerivativeOperations();
	if (strpos($operations, 'filmreel.png') === false) {
	    $operations .= (empty($operations) ? '' : ';') . "$op,top-left,0,0;$op,top-right,0,0";
	    $thumbnail->setDerivativeOperations($operations);
	}
    }

    function _removeWatermark(&$thumbnail) {
	$operations = $thumbnail->getDerivativeOperations();
	$thumbnail->setDerivativeOperations(rtrim(preg_replace(
	    '#(^|;)(composite\|[^;]*filmreel.png.*?(;|$))+#', '$1', $operations), ';'));
    }
}

/**
 * @package Ffmpeg
 * @subpackage UserInterface
 */
class AdminFfmpegView extends GalleryView {

    /**
     * @see GalleryView::loadTemplate
     */
    function loadTemplate(&$template, &$form) {
	global $gallery;

	$ret = GalleryCoreApi::assertUserIsSiteAdministrator();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	/* Load our default values if we didn't just come from this form. */
	if ($form['formName'] != 'AdminFfmpeg') {
	    foreach (array('path', 'useWatermark') as $key) {
		list ($ret, $form[$key]) =
		    GalleryCoreApi::getPluginParameter('module', 'ffmpeg', $key);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}
	    }
	    $form['formName'] = 'AdminFfmpeg';
	} else {
	    if (!isset($form['useWatermark'])) {
		$form['useWatermark'] = 0;
	    }
	}

	$platform =& $gallery->getPlatform();
	$tests = $mimeTypes = array();
	$debugSnippet = '';
	$failCount = 0;
	if (isset($form['action']['test'])) {
	    if (empty($form['path'])) {
		$form['error']['path']['missing'] = 1;
	    } else {
		$gallery->startRecordingDebugSnippet();
		list ($ret, $tests, $mimeTypes) = FfmpegToolkitHelper::testBinary($form['path']);
		$debugSnippet = $gallery->stopRecordingDebugSnippet();
		if ($ret) {
		    if ($ret->getErrorCode() & ERROR_BAD_PATH) {
			$form['error']['path']['badPath'] = 1;
			$tests = $mimeTypes = array();
		    } else {
			return array($ret->wrap(__FILE__, __LINE__), null);
		    }
		}

		foreach ($tests as $test) {
		    if (!$test['success']) {
			$failCount++;
		    }
		}
	    }
	}

	list ($ret, $toolkit) = GalleryCoreApi::getToolkitByOperation('image/jpeg', 'composite');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $module) = GalleryCoreApi::loadPlugin('module', 'ffmpeg');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}
	list ($ret, $isActive) = $module->isActive();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$AdminFfmpeg = array();
	$AdminFfmpeg['tests'] = $tests;
	$AdminFfmpeg['mimeTypes'] = $mimeTypes;
	$AdminFfmpeg['debugSnippet'] = $debugSnippet;
	$AdminFfmpeg['failCount'] = $failCount;
	$AdminFfmpeg['isConfigure'] = !$isActive;
	$AdminFfmpeg['canWatermark'] = isset($toolkit);

	if ($failCount > 0) {
	    $template->javascript('lib/javascript/BlockToggle.js');
	}

	$template->setVariable('AdminFfmpeg', $AdminFfmpeg);
	$template->setVariable('controller', 'ffmpeg.AdminFfmpeg');
	return array(null,
		     array('body' => 'modules/ffmpeg/templates/AdminFfmpeg.tpl'));
    }
}
?>
