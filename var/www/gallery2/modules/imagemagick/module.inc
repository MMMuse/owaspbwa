<?php
/*
 * $RCSfile: module.inc,v $
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
 * @version $Revision: 1.75.2.1 $ $Date: 2006/04/05 00:19:55 $
 * @package ImageMagick
 * @author Bharat Mediratta <bharat@menalto.com>
 * @author Vallimar <vallimar@sexorcisto.com>
 */

/**
 * ImageMagick Graphics Module
 *
 * This module provides the ImageMagick graphics toolkit for Gallery
 *
 * @package ImageMagick
 */
class ImageMagickModule extends GalleryModule {

    function ImageMagickModule() {
	global $gallery;

	$this->setId('imagemagick');
	$this->setName($gallery->i18n('ImageMagick'));
	$this->setDescription($gallery->i18n('ImageMagick / GraphicsMagick Graphics Toolkit'));
	$this->setVersion('1.0.7');
	$this->setGroup('toolkits', $gallery->i18n('Graphics Toolkits'));
	$this->setCallbacks('getSiteAdminViews');
	$this->setRequiredCoreApi(array(7, 0));
	$this->setRequiredModuleApi(array(3, 0));
    }

    /**
     * @see GalleryModule::upgrade
     */
    function upgrade($currentVersion) {
	list ($ret, $params) = GalleryCoreApi::fetchAllPluginParameters('module', 'imagemagick');
	if ($ret) {
	    return $ret->wrap(__FILE__, __LINE__);
	}
	foreach (array('path' => '', 'jpegQuality' => '75', 'cmykSupport' => 'none',
		       'versionOk' => '') as $key => $value) {
	    if (!isset($params[$key])) {
		$ret = $this->setParameter($key, $value);
		if ($ret) {
		    return $ret->wrap(__FILE__, __LINE__);
		}
	    }
	}

	if (!empty($params['path']) && empty($params['versionOk'])) {
	    GalleryCoreApi::requireOnce(
		'modules/imagemagick/classes/ImageMagickToolkitHelper.class');
	    list ($ret, $version, $vulnerable) =
		ImageMagickToolkitHelper::discoverVersion($params['path']);
	    if (!$ret && !$vulnerable) {
		$ret = $this->setParameter('versionOk', '1');
		if ($ret) {
		    return $ret->wrap(__FILE__, __LINE__);
		}
	    } else {
		list ($ret, $isActive) = $this->isActive();
		if ($ret) {
		    return $ret->wrap(__FILE__, __LINE__);
		}
		if ($isActive) {
		    /* Force upgrade from site admin to see vulnerable-version warning */
		    return GalleryCoreApi::error(ERROR_CONFIGURATION_REQUIRED, __FILE__, __LINE__);
		}
	    }
	}

	return null;
    }

    /**
     * @see GalleryModule::performFactoryRegistrations()
     */
    function performFactoryRegistrations() {
	/* Register our graphics class with the factory */
	$ret = GalleryCoreApi::registerFactoryImplementation(
	    'GalleryToolkit', 'ImageMagickToolkit', 'ImageMagick',
	    'modules/imagemagick/classes/ImageMagickToolkit.class', 'imagemagick', null);
	if ($ret) {
	    return $ret->wrap(__FILE__, __LINE__);
	}

	return null;
    }

    /**
     * @see GalleryModule::activate
     */
    function activate($postActivationEvent=true) {
	/* Load any classes we require */
	GalleryCoreApi::requireOnce(
	    'modules/imagemagick/classes/ImageMagickToolkitHelper.class');

	/* Find out what operations and properties we have available to us */
	list ($ret, $results) =
	    ImageMagickToolkitHelper::getOperationsAndProperties();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $priority) = GalleryCoreApi::getToolkitPriorityById('ImageMagick');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	if (!$priority) {
	    list ($ret, $priority) = GalleryCoreApi::getMaximumManagedToolkitPriority();
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }
	    $priority++;
	}

	foreach ($results['operations'] as $operation => $info) {
	    $outputMimeType = isset($info['outputMimeType']) ? $info['outputMimeType'] : '';
	    $ret = GalleryCoreApi::registerToolkitOperation('ImageMagick',
							    $info['mimeTypes'],
							    $operation,
							    $info['params'],
							    $info['description'],
							    $outputMimeType, $priority);
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }
	}

	/* Check CMYK colorspace support */
	list ($ret, $cmykSupport) = $this->getParameter('cmykSupport');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}
	if (!isset($results['properties']['colorspace']) && $cmykSupport != 'none') {
	    $ret = $this->setParameter('cmykSupport', $cmykSupport = 'none');
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }
	} else if (isset($results['properties']['colorspace']) && $cmykSupport == 'none') {
	    $ret = $this->setParameter('cmykSupport', $cmykSupport = 'off');
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }
	}

	foreach ($results['properties'] as $property => $info) {
	    if ($property == 'colorspace' && $cmykSupport != 'on') {
		/* Skip colorspace if this feature isn't turned on */
		continue;
	    }
	    $ret = GalleryCoreApi::registerToolkitProperty('ImageMagick',
							   $info['mimeTypes'],
							   $property,
							   $info['type'],
							   $info['description']);
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }
	}

	/* Save some platform specific ImageMagick parameters */
	$ret = ImageMagickToolkitHelper::savePlatformParameters();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $redirect) = parent::activate($postActivationEvent);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	return array(null, $redirect);
    }

    /**
     * @see GalleryModule::deactivate
     */
    function deactivate($postDeactivationEvent=true) {
	/*
	 * Unregister all of our properties and operations.  Do this before the parent deactivates
	 * so that any event handlers triggered by the deactivation will see the world as it will
	 * be after the deactivation is done.
	 */
	$ret = GalleryCoreApi::unregisterToolkit('ImageMagick');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $redirect) = parent::deactivate($postDeactivationEvent);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	return array(null, $redirect);
    }

    /**
     * @see GalleryModule::needsConfiguration
     */
    function needsConfiguration() {
	/* This module requires all fields to be filled out before it can be activated. */
	foreach (array('path', 'jpegQuality', 'cmykSupport', 'versionOk') as $key) {
	    list ($ret, $value) = $this->getParameter($key);
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }

	    if (empty($value)) {
		return array(null, true);
	    }
	}

	return array(null, false);
    }

    /**
     * @see GalleryModule::autoConfigure
     */
    function autoConfigure() {
	global $gallery;

	list ($ret, $needsConfiguration) = $this->needsConfiguration();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), false);
	}

	if (!$needsConfiguration) {
	    return array(null, true);
	}

	/* Try a bunch of likely seeming paths to see if any of them work. */
	$platform =& $gallery->getPlatform();
	$slash = $platform->getDirectorySeparator();

	/*
	 * Start with system paths.  Tack on a trailing slash if necessary,
	 * then tack on other likely paths, based on our OS
	 */
	$paths = array();
	if (GalleryUtilities::isA($platform, 'WinNtPlatform')) {
	    foreach (explode(';', getenv('PATH')) as $path) {
		$path = trim($path);
		if (empty($path)) {
		    continue;
		}
		if ($path{strlen($path)-1} != $slash) {
		    $path .= $slash;
		}
		$paths[] = $path;
	    }

	    /*
	     * Double-quoting the paths removes any ambiguity about the
	     * double-slashes being escaped or not
	     */
	    $paths[] = "C:\\Program Files\\ImageMagick\\";
	    $paths[] = "C:\\apps\ImageMagick\\";
	    $paths[] = "C:\\ImageMagick\\";
	    $paths[] = "C:\\ImageMagick\\VisualMagick\\bin\\";
	    $paths[] = "C:\\cygwin\\bin\\";
	} else if (GalleryUtilities::isA($platform, 'UnixPlatform')){
	    foreach (explode(':', getenv('PATH')) as $path) {
		$path = trim($path);
		if (empty($path)) {
		    continue;
		}
		if ($path{strlen($path)-1} != $slash) {
		    $path .= $slash;
		}
		$paths[] = $path;
	    }

	    $paths[] = '/usr/bin/';
	    $paths[] = '/usr/local/bin/';
	    $paths[] = '/bin/';
	    $paths[] = '/sw/bin/';
	} else {
	    return array(null, false);
	}

	/* Load any classes we require */
	GalleryCoreApi::requireOnce(
	    'modules/imagemagick/classes/ImageMagickToolkitHelper.class');

	/* Now try each path in turn to see which ones work */
	foreach ($paths as $path) {
	    list ($ret, $testResults) = ImageMagickToolkitHelper::testBinaries($path);
	    if ($ret) {
		/* This path failed.  Continue with the next one */
		continue;
	    }

	    $failCount = 0;
	    foreach ($testResults as $testResult) {
		/* All tests should work, else this path is not a valid one */
		if (!$testResult['success']) {
		    $failCount++;
		}
	    }

	    if ($failCount == 0) {
		/* We have a winner */
		$ret = GalleryCoreApi::setPluginParameter('module', 'imagemagick', 'path', $path);
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), false);
		}

		/* Force admin to visit site admin to view warning for vulnerable version */
		list ($ret, $version, $vulnerable) =
		    ImageMagickToolkitHelper::discoverVersion($path);
		if (!$ret && !$vulnerable) {
		    $ret = GalleryCoreApi::setPluginParameter('module', 'imagemagick',
							      'versionOk', '1');
		    if ($ret) {
			return array($ret->wrap(__FILE__, __LINE__), false);
		    }
		} else {
		    $vulnerable = true;
		}
		return array(null, !$vulnerable);
	    }
	}

	return array(null, false);
    }

    /**
     * @see GalleryModule::getSiteAdminViews
     */
    function getSiteAdminViews() {
	return array(null,
		     array(array('name' => $this->translate('ImageMagick'),
				 'view' => 'imagemagick.AdminImageMagick')));
    }

    /**
     * @see GalleryModule::getConfigurationView
     */
    function getConfigurationView() {
	return 'imagemagick.AdminImageMagick';
    }
}
?>
