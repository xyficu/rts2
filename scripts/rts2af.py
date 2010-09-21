"""Class definitions for rts2_autofocus"""
# (C) 2010, Markus Wildi, markus.wildi@one-arcsec.org
#
#   usage 
#   rts_autofocus.py --help
#   
#   see man 1 rts2_autofocus.py
#   see rts2_autofocus_unittest.py for unit tests
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#
# required packages:
# wget 'http://argparse.googlecode.com/files/argparse-1.1.zip

__author__ = 'markus.wildi@one-arcsec.org'

import argparse
import logging
from operator import itemgetter
# globals
LOG_FILENAME = '/var/log/rts2-autofocus'
logger= logging.getLogger('rts2_af_logger') ;


class AFScript:
    """Class for any AF script"""
    def __init__(self):
        self.debug = False

    def arguments( self): 

        parser = argparse.ArgumentParser(description='RTS2 autofocus', epilog="See man 1 rts2-autofocus.py for mor information")
#        parser.add_argument(
#            '--write', action='store', metavar='FILE NAME', 
#            type=argparse.FileType('w'), 
#            default=sys.stdout,
#            help='the file name where the default configuration will be written default: write to stdout')

        parser.add_argument(
            '-w', '--write', action='store_true', 
            help='write defaults to configuration file ' + runTimeConfig.configFileName)

        parser.add_argument('--config', dest='fileName',
                            metavar='CONFIG FILE', nargs=1, type=str,
                            help='configuration file')

        parser.add_argument('-r', '--reference', dest='referenceFitsFileName',
                            metavar='REFERENCE', nargs=1, type=str,
                            help='reference file name')

        parser.add_argument('-l', '--logging', dest='logLevel', action='store', 
                            metavar='LOGLEVEL', nargs=1, type=str,
                            default='warning',
                            help=' log level: usual levels')

#        parser.add_argument('-t', '--test', dest='test', action='store', 
#                            metavar='TEST', nargs=1,
#    no default means None                        default='myTEST',
#                            help=' test case, default: myTEST')

        parser.add_argument('-v', dest='verbose', 
                            action='store_true',
                            help=' print (some) messages to stdout'
                            )

        self.args= parser.parse_args()

        if(self.args.verbose):
            global verbose
            verbose= self.args.verbose
            if( verbose):
                runTimeConfig.dumpDefaults()

        if( self.args.write):
            runTimeConfig.writeDefaultConfiguration()
            print 'wrote default configuration to ' +  runTimeConfig.configurationFileName()
            sys.exit(0)

        self.debug = self.args.logLevel[0]== 'debug'

        return  self.args

    def configureLoggerStdout(self):
        """Send logging output to stdout."""
        ch = logging.StreamHandler()
        if self.debug:
            ch.setLevel(logging.DEBUG)
        else:
            ch.setLevel(logging.WARN)
        logger.addHandler(ch)           

    def configureLogger(self):
        if self.debug:
            logging.basicConfig(filename=LOG_FILENAME,level=logging.DEBUG)
        else:
            logging.basicConfig(filename=LOG_FILENAME,level=logging.WARN)

        return logger

import ConfigParser
import string

class Configuration:
    """Configuration for any AFScript"""    
    def __init__(self, fileName='rts2-autofocus-offline.cfg'):
        self.configFileName = fileName
        self.values={}
        self.filters=[]
        self.filtersInUse=[]

        self.cp={
            ('basic', 'CONFIGURATION_FILE')                          : '/etc/rts2/autofocus/rts2-autofocus.cfg',
            ('basic', 'BASE_DIRECTORY')                              : '/tmp',
            ('basic', 'TEMP_DIRECTORY')                              : '/tmp',
            ('basic', 'FILE_GLOB')                                   : '*fits',
            ('basic', 'FITS_IN_BASE_DIRECTORY')                      : False,
            ('basic', 'CCD_CAMERA')                                  : 'CD',
            ('basic', 'CHECK_RTS2_CONFIGURATION')                    : False,
    
            ('filter properties', 'U')                               : '[0, U, 5074, -1500, 1500, 100, 40]',
            ('filter properties', 'B')                               : '[1, B, 4712, -1500, 1500, 100, 30]',
            ('filter properties', 'V')                               : '[2, V, 4678, -1500, 1500, 100, 20]',
            ('filter properties', 'R')                               : '[4, R, 4700, -1500, 1500, 100, 20]',
            ('filter properties', 'I')                               : '[4, I, 4700, -1500, 1500, 100, 20]',
            ('filter properties', 'X')                               : '[5, X, 3270, -1500, 1500, 100, 10]',
            ('filter properties', 'Y')                               : '[6, Y, 3446, -1500, 1500, 100, 10]',
            ('filter properties', 'NOFILTER')                        : '[6, NOFILTER, 3446, -1500, 1500, 109, 19]',
            
            ('focuser properties', 'FOCUSER_RESOLUTION')             : 20,
            ('focuser properties', 'FOCUSER_ABSOLUTE_LOWER_LIMIT')   : 1501,
            ('focuser properties', 'FOCUSER_ABSOLUTE_UPPER_LIMIT')   : 6002,
    
            ('acceptance circle', 'CENTER_OFFSET_X')                 : 0.,
            ('acceptance circle', 'CENTER_OFFSET_Y')                 : 0.,
            ('acceptance circle', 'RADIUS')                          : 2000.,
            
            ('filters', 'filters')                                   : 'U:B:V:R:I:X:Y',
            
            ('DS9', 'DS9_REFERENCE')                                 : True,
            ('DS9', 'DS9_MATCHED')                                   : True,
            ('DS9', 'DS9_ALL')                                       : True,
            ('DS9', 'DS9_DISPLAY_ACCEPTANCE_AREA')                   : True,
            ('DS9', 'DS9_REGION_FILE')                               : 'ds9-autofocus.reg',
            
            ('analysis', 'ANALYSIS_UPPER_LIMIT')                     : 1.e12,
            ('analysis', 'ANALYSIS_LOWER_LIMIT')                     : 0.,
            ('analysis', 'MINIMUM_OBJECTS')                          : 20,
            ('analysis', 'MINIMUM_FOCUSER_POSITIONS')                : 5,
            ('analysis', 'INCLUDE_AUTO_FOCUS_RUN')                   : False,
            ('analysis', 'SET_LIMITS_ON_SANITY_CHECKS')              : True,
            ('analysis', 'SET_LIMITS_ON_FILTER_FOCUS')               : True,
            ('analysis', 'FIT_RESULT_FILE')                          : 'fit-autofocus.dat',
            ('analysis', 'MATCHED_RATIO')                            : 0.1,
            
            ('fitting', 'FOCROOT')                                   : 'rts2-fit-focus',
            
            ('SExtractor', 'SEXPRG')                                 : 'sextractor 2>/dev/null',
            ('SExtractor', 'SEXCFG')                                 : '/etc/rts2/autofocus/sex-autofocus.cfg',
            ('SExtractor', 'SEXPARAM')                               : '/etc/rts2/autofocus/sex-autofocus.param',
            ('SExtractor', 'SEXREFERENCE_PARAM')                     : '/etc/rts2/autofocus/sex-autofocus-reference.param',
            ('SExtractor', 'OBJECT_SEPARATION')                      : 10.,
            ('SExtractor', 'ELLIPTICITY')                            : .1,
            ('SExtractor', 'ELLIPTICITY_REFERENCE')                  : .3,
            ('SExtractor', 'SEXSKY_LIST')                            : 'sex-autofocus-assoc-sky.list',
            ('SExtractor', 'SEXCATALOGUE')                           : 'sex-autofocus.cat',
            ('SExtractor', 'SEX_TMP_CATALOGUE')                      : 'sex-autofocus-tmp.cat',
            ('SExtractor', 'CLEANUP_REFERENCE_CATALOGUE')            : True,
            
            ('mode', 'TAKE_DATA')                                    : False,
            ('mode', 'SET_FOCUS')                                    : True,
            ('mode', 'CCD_BINNING')                                  : 0,
            ('mode', 'AUTO_FOCUS')                                   : False,
            ('mode', 'NUMBER_OF_AUTO_FOCUS_IMAGES')                  : 10,
            
            ('rts2', 'RTS2_DEVICES')                                 : '/etc/rts2/devices'
	}

        self.config = ConfigParser.RawConfigParser()
        
        self.defaults={}
        for (section, identifier), value in sorted(self.cp.iteritems()):
            self.defaults[(identifier)]= value
#            print 'value ', self.defaults[(identifier)]
 
    def configIdentifiers(self):
        return sorted(self.cp.iteritems())

    def defaultsIdentifiers(self):
        return sorted(self.defaults.iteritems())
        
    def defaultsValue( self, identifier):
        return self.defaults[ identifier]

    def dumpDefaults(self):
        for (identifier), value in self.configIdentifiers():
            print "dump defaults :", ', ', identifier, '=>', value

    def valuesIdentifiers(self):
        return sorted(self.values.iteritems())
    # why not values?
    #  runTimeConfig.values('SEXCFG'), TypeError: 'dict' object is not callable
    def value( self, identifier):
        return self.values[ identifier]

    def dumpValues(self):
        for (identifier), value in self.valuesIdentifiers():
            print "dump values :", ', ', identifier, '=>', value

    def filterByName(self, name):
        for filter in  self.filters:
            print "NAME>" + name + "<>" + filter.filterName
            if( name == filter.filterName):
                print "NAME" + name + filter.filterName
                return filter

        return False

    def filterInUseByName(self, name):
        for filter in  self.filtersInUse:
            if( name == filter.filterName):
                return filter
        return False

    def writeDefaultConfiguration(self):
        for (section, identifier), value in sorted(self.cp.iteritems()):
#            print section, '=>', identifier, '=>', value
            if( self.config.has_section(section)== False):
                self.config.add_section(section)

            self.config.set(section, identifier, value)

        with open(self.configFileName, 'wb') as configfile:
            configfile.write('# 2010-07-10, Markus Wildi\n')
            configfile.write('# default configuration for rts2-autofocus.py\n')
            configfile.write('# generated with rts2-autofous.py -p\n')
            configfile.write('# see man rts2-autofocus.py for details\n')
            configfile.write('#\n')
            configfile.write('#\n')
            self.config.write(configfile)


    def readDefaults(self):
        # read the defaults
        for (section, identifier), value in self.configIdentifiers():
             self.values[identifier]= value

    def readConfiguration(self, configFileName):

        config = ConfigParser.ConfigParser()
        try:
            config.readfp(open(configFileName))
        except:
            logger.error('Configuration.readConfiguration: config file %s not found, exiting' % configFileName)
            sys.exit(1)

        self.readDefaults()

        # over write the defaults
        for (section, identifier), value in self.configIdentifiers():

            try:
                value = config.get( section, identifier)
            except:
                logger.info('Configuration.readConfiguration: no section ' +  section + ' or identifier ' +  identifier + ' in file ' + configFileName)
            # first bool, then int !
            if(isinstance(self.values[identifier], bool)):
            #ToDO, looking for a direct way
                if( value == 'True'):
                    self.values[identifier]= True
                else:
                    self.values[identifier]= False
            elif( isinstance(self.values[identifier], int)):
                try:
                    self.values[identifier]= int(value)
                except:
                    logger.error('Configuration.readConfiguration: no int '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)
                    

            elif(isinstance(self.values[identifier], float)):
                try:
                    self.values[identifier]= float(value)
                except:
                    logger.error('Configuration.readConfiguration: no float '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + configFileName)

            else:
                self.values[identifier]= value
                items=[] ;
                if( section == 'filter properties'): 
                    items= value[1:-1].split(',')
#, ToDo, hm
                    for item in items: 
                        item= string.replace( item, ' ', '')

                    items[1]= string.replace( items[1], ' ', '')
                    if(verbose):
                        print '-----------filterInUse---------:>' + items[1] + '<'
                    self.filters.append( Filter( items[1], string.atoi(items[2]), string.atoi(items[3]), string.atoi(items[4]), string.atoi(items[5]), string.atoi(items[6])))
                elif( section == 'filters'):
                    items= value.split(':')
                    for item in items:
                        item.replace(' ', '')
                        if(verbose):
                            print 'filterInUse---------:>' + item + '<'
                        self.filtersInUse.append(item)

        if(verbose):
            for (identifier), value in self.defaultsIdentifiers():
               print "over ", identifier, self.values[(identifier)]

        if(verbose):
           for filter in self.filters:
               print 'Filter --------' + filter.filterName
               print 'Filter --------%d'  % filter.exposure 

        if(verbose):
            for filter in self.filtersInUse:
                print "used filters :", filter

        return self.values

class SExtractorParams():
    def __init__(self, paramsFileName=None):
# ToDo, clarify 
        if( paramsFileName== None):
            self.paramsFileName= runTimeConfig.value('SEXREFERENCE_PARAM')
        else:
            self.paramsFileName= paramsFileName
        self.reference= []
        self.assoc= []

    def elementIndex(self, element):
        for ele in  self.assoc:
            if( element== ele) : 
                return self.assoc.index(ele)
        else:
            return False

    def readSExtractorParams(self):
        params=open( self.paramsFileName, 'r')
        lines= params.readlines()
        pElement = re.compile( r'([\w]+)')
        for (i, line) in enumerate(lines):
            element = pElement.match(line)
            if( element):
                if(verbose):
                    print "element.group(1) : ", element.group(1)
                self.reference.append(element.group(1))
                self.assoc.append(element.group(1))

# current structure of the reference catalogue
#   1 NUMBER                 Running object number
#   2 X_IMAGE                Object position along x                                    [pixel]
#   3 Y_IMAGE                Object position along y                                    [pixel]
#   4 FLUX_ISO               Isophotal flux                                             [count]
#   5 FLUX_APER              Flux vector within fixed circular aperture(s)              [count]
#   6 FLUXERR_APER           RMS error vector for aperture flux(es)                     [count]
#   7 MAG_APER               Fixed aperture magnitude vector                            [mag]
#   8 MAGERR_APER            RMS error vector for fixed aperture mag.                   [mag]
#   9 FLUX_MAX               Peak flux above background                                 [count]
#  10 ISOAREA_IMAGE          Isophotal area above Analysis threshold                    [pixel**2]
#  11 FLAGS                  Extraction flags
#  12 FWHM_IMAGE             FWHM assuming a gaussian core                              [pixel]
#  13 FLUX_RADIUS            Fraction-of-light radii                                    [pixel]
#  14 ELLIPTICITY            1 - B_IMAGE/A_IMAGE

# current structure of the associated catalogue
#   1 NUMBER                 Running object number
#   2 X_IMAGE                Object position along x                                    [pixel]
#   3 Y_IMAGE                Object position along y                                    [pixel]
#   4 FLUX_ISO               Isophotal flux                                             [count]
#   5 FLUX_APER              Flux vector within fixed circular aperture(s)              [count]
#   6 FLUXERR_APER           RMS error vector for aperture flux(es)                     [count]
#   7 MAG_APER               Fixed aperture magnitude vector                            [mag]
#   8 MAGERR_APER            RMS error vector for fixed aperture mag.                   [mag]
#   9 FLUX_MAX               Peak flux above background                                 [count]
#  10 ISOAREA_IMAGE          Isophotal area above Analysis threshold                    [pixel**2]
#  11 FLAGS                  Extraction flags
#  12 FWHM_IMAGE             FWHM assuming a gaussian core                              [pixel]
#  13 FLUX_RADIUS            Fraction-of-light radii                                    [pixel]
#  14 ELLIPTICITY            1 - B_IMAGE/A_IMAGE
#  15 VECTOR_ASSOC           ASSOCiated parameter vector
#  29 NUMBER_ASSOC           Number of ASSOCiated IDs
            
        for element in self.reference:
            self.assoc.append('ASSOC_'+ element)

        self.assoc.append('NUMBER_ASSOC')

        if(verbose):
            for element in  self.reference:
                print "Reference element : >"+ element+"<"
            for element in  self.assoc:
                print "Association element : >" + element+"<"

class Filter:
    """Class for filter properties"""

    def __init__(self, filterName, focDef, lowerLimit, upperLimit, resolution, exposure, ):
        self.filterName= filterName
        self.focDef    = focDef
        self.lowerLimit= lowerLimit
        self.upperLimit= upperLimit
        self.stepSize  = resolution # [tick]
        self.exposure  = exposure


class SXObject():
    """Class holding the used properties of SExtractor object"""
    def __init__(self, objectNumber=None, focusPosition=None, position=None, fwhm=None, flux=None, associatedSXobject=None, separationOK=True, propertiesOK=True, acceptanceOK=True):
        self.objectNumber= objectNumber
        self.focusPosition= focusPosition
        self.position= position # is a list (x,y)
        self.matched= False
        self.fwhm= fwhm
        self.flux= flux
        self.associatedSXobject= associatedSXobject
        self.separationOK= separationOK
        self.propertiesOK= propertiesOK
        self.acceptanceOK= acceptanceOK

    def printPosition(self):
        print "=== %f %f" %  (self.x, self.y)
    
    def isValid(self):
        if (self.objectNumber != None and self.x != None and self.y != None and self.fwhm != None and self.flux != None):
            return True
        return False


class SXReferenceObject(SXObject):
    """Class holding the used properties of SExtractor object"""
    def __init__(self, objectNumber=None, focusPosition=None, position=None, fwhm=None, flux=None, separationOK=True, propertiesOK=True, acceptanceOK=True):
        self.objectNumber= objectNumber
        self.focusPosition= focusPosition
        self.position= position # is a list (x,y)
        self.matchedReference= False
        self.foundInAll= False
        self.fwhm= fwhm
        self.flux= flux
        self.separationOK= separationOK
        self.propertiesOK= propertiesOK
        self.acceptanceOK= acceptanceOK
        self.multiplicity=0
        self.numberOfMatches=0
        self.matchedsxObjects=[]

    def __str__(self):
        return '{0}: {1} {2} {3}'.format(self.objectNumber, self.position, self.fwhm, self.flux)


    def append(self, sxObject):
        self.matchedsxObjects.append(sxObject)
        

import shlex
import subprocess
import re
from math import sqrt
class Catalogue():
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, fitsHDU=None, SExtractorParams=None, referenceCatalogue=None):
        self.fitsHDU  = fitsHDU
        self.catalogueFileName= serviceFileOp.expandToCat(self.fitsHDU)
        self.lines= []
        self.catalogue = {}
        self.sxObjects = {}
        self.multiplicity = {}
        self.isReference = False
        self.isValid     = False
        self.SExtractorParams = SExtractorParams
        self.referenceCatalogue= referenceCatalogue
        self.indexeFlag       = self.SExtractorParams.assoc.index('FLAGS')  
        self.indexellipticity = self.SExtractorParams.assoc.index('ELLIPTICITY')
        
        Catalogue.__lt__ = lambda self, other: self.fitsHDU.headerElements['FOC_POS'] < other.fitsHDU.headerElements['FOC_POS']

    def runSExtractor(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        # 2>/dev/null swallowed by PIPE
        (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')

        cmd= [  prg,
                self.fitsHDU.fitsFileName, 
                '-c ',
                runTimeConfig.value('SEXCFG'),
                '-CATALOG_NAME',
                self.catalogueFileName,
                '-PARAMETERS_NAME',
                runTimeConfig.value('SEXPARAM'),
                '-ASSOC_NAME',
                serviceFileOp.expandToSkyList(self.fitsHDU)
                ]
        try:
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

        except OSError as (errno, strerror):
            logging.error( 'Catalogue.runSExtractor: I/O error({0}): {1}'.format(errno, strerror))
            sys.exit(1)

        except:
            logging.error('Catalogue.runSExtractor: '+ repr(cmd) + ' died')
            sys.exit(1)

# checking against reference items see http://www.wellho.net/resources/ex.php4?item=y115/re1.py
    def createCatalogue(self):
        if( self.SExtractorParams==None):
            logger.error( 'Catalogue.createCatalogue: no SExtractor parameter configuration given')
            return False

        #if( not self.fitsHDU.isReference):
        SXcat= open( self.catalogueFileName, 'r')
        #else:
        #    SXcat= open( serviceFileOp.expandToSkyList(self.fitsHDU), 'r')

        self.lines= SXcat.readlines()
        SXcat.close()

        # rely on the fact that first line is stored as first element in list
        # match, element, data
        ##   2 X_IMAGE                Object position along x                                    [pixel]
        #         1   2976.000      7.473     2773.781     219.8691     42.19405  -5.8554   0.2084     120.5605         6  26     4.05      5.330    0.823
 
        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'')
        sxObjectNumber= -1
        itemNrX_IMAGE     = self.SExtractorParams.assoc.index('X_IMAGE')
        itemNrY_IMAGE     = self.SExtractorParams.assoc.index('Y_IMAGE')
        itemNrFWHM_IMAGE  = self.SExtractorParams.assoc.index('FWHM_IMAGE')
        itemNrFLUX_MAX    = self.SExtractorParams.assoc.index('FLUX_MAX')
        itemNrASSOC_NUMBER= self.SExtractorParams.assoc.index('ASSOC_NUMBER')

        for (lineNumber, line) in enumerate(self.lines):
            element = pElement.match(line)
            data    = pData.match(line)
            x= -1
            y= -1
            sxObjectNumberASSOC= '-1'
            if( element):
                if( not ( element.group(2)== 'VECTOR_ASSOC')):
                    try:
                        self.SExtractorParams.assoc.index(element.group(2))
                    except:
                        logger.error( 'Catalogue.createCatalogue: no matching element for ' + element.group(2) +' found')
			return False
            elif( data):
                items= line.split()
                sxObjectNumber= items[0] # NUMBER
                sxObjectNumberASSOC= items[itemNrASSOC_NUMBER]
                
                for (j, item) in enumerate(items):
                    self.catalogue[(sxObjectNumber, self.SExtractorParams.assoc[j])]=  float(item)


                self.sxObjects[sxObjectNumber]= SXObject(sxObjectNumber, self.fitsHDU.headerElements['FOC_POS'], (float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE])), float(items[itemNrFWHM_IMAGE]), float(items[itemNrFLUX_MAX]), items[itemNrASSOC_NUMBER]) # position, bool is used for clean up (default True, True)
                if( sxObjectNumberASSOC in self.multiplicity): # interesting
                    self.multiplicity[sxObjectNumberASSOC] += 1
                else:
                    self.multiplicity[sxObjectNumberASSOC]= 1
            else:
                logger.error( 'Catalogue.readCatalogue: no match on line %d' % lineNumber)
                logger.error( 'Catalogue.readCatalogue: ' + line)
		return False

        logger.info('Catalogue.readCatalogue: catalogue for {0} read from {1}'.format(self.fitsHDU.fitsFileName, self.catalogueFileName))
        self.isValid= True

        return self.isValid

    def printCatalogue(self):
        for (sxObjectNumber,identifier), value in sorted( self.catalogue.iteritems()):
# ToDo, remove me:
            if( sxObjectNumber== "12"):
                print  "printCatalogue " + sxObjectNumber + ">"+ identifier + "< %f"% value 


    def printObject(self, sxObjectNumber):
        for itentifier in self.SExtractorParams.identifiersAssoc:
            if(((sxObjectNumber, itentifier)) in self.catalogue):
                if(verbose):
                    print "printObject: object number " + sxObjectNumber + " identifier " + itentifier + "value %f" % self.catalogue[(sxObjectNumber, itentifier)]
            else:
                logger.error( "Catalogue.printObject: object number " + sxObjectNumber + " for >" + itentifier + "< not found, break")
                break
        else:
#                logger.error( "Catalogue.printObject: for object number " + sxObjectNumber + " all elements printed")
            return True

        return False

    def removeSXObject(self, sxObjectNumber):
        if( not sxObjectNumber in self.sxObjects):
            logger.error( "Catalogue.removeSXObject: reference Object number " + sxObjectNumber + " for >" + itentifier + "< not found in sxObjects")
        else:
            if( sxObjectNumber in self.sxObjects):
                del self.sxObjects[sxObjectNumber]
            else:
                logger.error( "Catalogue.removeSXObject: object number " + sxObjectNumber + " not found")

        for itentifier in self.SExtractorParams.reference:
            if(((sxObjectNumber, itentifier)) in self.catalogue):
                del self.catalogue[(sxObjectNumber, itentifier)]
            else:
                logger.error( "Catalogue.removeSXObject: object number " + sxObjectNumber + " for >" + itentifier + "< not found, break")
                break
        else:
#                logger.error( "Catalogue.removeSXObject: for object number " + sxObjectNumber + " all elements deleted")
            return True
        return False

    # now done on catalogue for the sake of flexibility
    # ToDo, define if that shoud go into SXObject (now, better not)
    def checkProperties(self, sxObjectNumber): 
        if( self.catalogue[( sxObjectNumber, 'FLAGS')] != 0): # ToDo, ATTENTION
            #print "checkProperties flags %d" % self.catalogue[( sxObjectNumber, 'FLAGS')]
            return False
        elif( self.catalogue[( sxObjectNumber, 'ELLIPTICITY')] > runTimeConfig.value('ELLIPTICITY')): # ToDo, ATTENTION
            #print "checkProperties elli %f %f" %  (self.catalogue[( sxObjectNumber, 'ELLIPTICITY')],runTimeConfig.value('ELLIPTICITY'))
            return False
        # TRUE    
        return True

    def cleanUp(self):
        logger.error( 'Catalogue.cleanUp: catalogue, I am : ' + self.fitsHDU.fitsFileName)

        discardedObjects= 0
        sxObjectNumbers= self.sxObjects.keys()
        for sxObjectNumber in sxObjectNumbers:
            if( not self.checkProperties(sxObjectNumber)):
                # self.removeSXObject( sxObjectNumber)
                discardedObjects += 1

        logger.error("Catalogue.cleanUp: Number of objects discarded %d " % discardedObjects) 

    def matching(self, ReferenceCatalogue):
        matched= 0
        for sxObjectNumber, sxObject in self.sxObjects.items():
                sxReferenceObject= ReferenceCatalogue.sxObjectByNumber(sxObject.associatedSXobject)
                if( sxReferenceObject != None):
                    if( self.multiplicity[sxReferenceObject.objectNumber] == 1): 
                        matched += 1
                        sxObject.matched= True 
                        sxReferenceObject.numberOfMatches += 1
                        sxReferenceObject.matchedsxObjects.append(sxObject)
                    else:
                        pass
                        #if( verbose):
                        #    print "lost " + sxReferenceObject.objectNumber + " %d" % self.multiplicity[sxReferenceObject.objectNumber] # count

        if(verbose):
            print "number of matched sxObjects %d=" % matched + "of %d "% len(ReferenceCatalogue.sxObjects) + " required %f " % ( runTimeConfig.value('MATCHED_RATIO') * len(ReferenceCatalogue.sxObjects))

        if( (float(matched)/float(len(ReferenceCatalogue.sxObjects))) > runTimeConfig.value('MATCHED_RATIO')):
            return True
        else:
            logger.error("main: too few sxObjects matched %d" % matched + " of %d" % len(ReferenceCatalogue.sxObjects) + " required are %f" % ( runTimeConfig.value('MATCHED_RATIO') * len(ReferenceCatalogue.sxObjects)) + " sxobjects at FOC_POS+ %d= " % self.fitsHDU.headerElements['FOC_POS'] + "file "+ self.fitsHDU.fitsFileName)
            return False


# example how to access the catalogue
    def average(self, variable):
        sum= 0
        i= 0
        for (sxObjectNumber,identifier), value in self.catalogue.iteritems():
            if( identifier == variable):
                sum += float(value)
                i += 1

        #if(verbose):
        if( i != 0):
            print 'average ' + variable + ' %f ' % (sum/ float(i)) 
            return (sum/ float(i))
        else:
            print 'Error in average i=0'
            return False

class ReferenceCatalogue(Catalogue):
    """Class for a catalogue (SExtractor result)"""
    def __init__(self, fitsHDU=None, SExtractorParams=None):
        self.fitsHDU  = fitsHDU
        self.catalogueFileName= serviceFileOp.expandToCat(self.fitsHDU)
        self.lines= []
        self.catalogue = {}
        self.sxObjects = {}
        self.isReference = False
        self.isValid     = False
        self.objectSeparation2= runTimeConfig.value('OBJECT_SEPARATION')**2
        self.SExtractorParams = SExtractorParams
        self.indexeFlag       = self.SExtractorParams.reference.index('FLAGS')  
        self.indexellipticity = self.SExtractorParams.reference.index('ELLIPTICITY')
        self.skyList= serviceFileOp.expandToSkyList(self.fitsHDU)
        self.circle= AcceptanceRegion( self.fitsHDU) 

        ReferenceCatalogue.__lt__ = lambda self, other: self.fitsHDU.headerElements['FOC_POS'] < other.fitsHDU.headerElements['FOC_POS']

    def sxObjectByNumber(self, sxObjectNumber):
        if(sxObjectNumber in self.sxObjects):
            return self.sxObjects[sxObjectNumber]
        return None

    def writeCatalogue(self):
        logger.error( 'ReferenceCatalogue.writeCatalogue: writing reference catalogue, for ' + self.fitsHDU.fitsFileName) 

        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'^[ \t]+([0-9]+)[ \t]+')
        SXcat= open( self.skyList, 'wb')

        for line in self.lines:
            element= pElement.search(line)
            data   = pData.search(line)
            if(element):
                SXcat.write(line)
            else:
                if((data.group(1)) in self.sxObjects):
                    try:
                        SXcat.write(line)
                    except:
                        logger.error( 'ReferenceCatalogue.writeCatalogue: could not write line for object ' + data.group(1))
                        sys.exit(1)
                        break
        SXcat.close()

    def runSExtractor(self):
        if( verbose):
            print 'sextractor  ' + runTimeConfig.value('SEXPRG')
            print 'sextractor  ' + runTimeConfig.value('SEXCFG')
            print 'sextractor  ' + runTimeConfig.value('SEXREFERENCE_PARAM')

        # 2>/dev/null swallowed by PIPE
        (prg, arg)= runTimeConfig.value('SEXPRG').split(' ')

        self.isReference = True 
        cmd= [  prg,
                self.fitsHDU.fitsFileName, 
                '-c ',
                runTimeConfig.value('SEXCFG'),
                '-CATALOG_NAME',
                self.skyList,
                '-PARAMETERS_NAME',
                runTimeConfig.value('SEXREFERENCE_PARAM'),]
        try:
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]

        except OSError as (errno, strerror):
            logging.error( 'ReferenceCatalogue.runSExtractor {0}: I/O error({1}): {2}'.format(prg, errno, strerror))
            sys.exit(1)

        except:
            logging.error('ReferenceCatalogue.runSExtractor: '+ repr(cmd) + ' died')
            sys.exit(1)

# checking against reference items see http://www.wellho.net/resources/ex.php4?item=y115/re1.py
    def createCatalogue(self):
        if( self.SExtractorParams==None):
            logger.error( 'ReferenceCatalogue.createCatalogue: no SExtractor parameter configuration given')
            return False

        # if( not self.fitsHDU.isReference):
        #SXcat= open( self.catalogueFileName, 'r')
        #else:
        try:
            SXcat= open( self.skyList, 'r')
        except Exception (ex):
            logger.error("could not open file {0}: {1}".format(self.skyList, ex))
            sys.exit(1) 

        self.lines= SXcat.readlines()
        SXcat.close()

        # rely on the fact that first line is stored as first element in list
        # match, element, data
        ##   2 X_IMAGE                Object position along x                                    [pixel]
        #         1   2976.000      7.473     2773.781     219.8691     42.19405  -5.8554   0.2084     120.5605         6  26     4.05      5.330    0.823
 
        pElement = re.compile( r'#[ ]+([0-9]+)[ ]+([\w]+)')
        pData    = re.compile( r'')
        sxObjectNumber= -1
        itemNrX_IMAGE     = self.SExtractorParams.reference.index('X_IMAGE')
        itemNrY_IMAGE     = self.SExtractorParams.reference.index('Y_IMAGE')
        itemNrFWHM_IMAGE  = self.SExtractorParams.reference.index('FWHM_IMAGE')
        itemNrFLUX_MAX    = self.SExtractorParams.reference.index('FLUX_MAX')

        for (lineNumber, line) in enumerate(self.lines):
            element = pElement.match(line)
            data    = pData.match(line)
            x= -1
            y= -1
            sxObjectNumberASSOC= '-1'
            if( element):
                pass
            elif( data):
                items= line.split()
                sxObjectNumber= items[0] # NUMBER
                
                for (j, item) in enumerate(items):
                    self.catalogue[(sxObjectNumber, self.SExtractorParams.reference[j])]=  float(item)


                self.sxObjects[sxObjectNumber]= SXReferenceObject(sxObjectNumber, self.fitsHDU.headerElements['FOC_POS'], (float(items[itemNrX_IMAGE]), float(items[itemNrY_IMAGE])), float(items[itemNrFWHM_IMAGE]), float(items[itemNrFLUX_MAX])) # position, bool is used for clean up (default True, True)
            else:
                logger.error('ReferenceCatalogue.readCatalogue: no match on line %d' % lineNumber)
                logger.error('ReferenceCatalogue.readCatalogue: ' + line)
                break

        logger.error('ReferenceCatalogue.readCatalogue: catalogue for file {0} created in {1}'.format(self.fitsHDU.fitsFileName, self.catalogueFileName))
        self.isValid= True

        return self.isValid

    def printObject(self, sxObjectNumber):
            
        for itentifier in self.SExtractorParams.reference():
            
            if(((sxObjectNumber, itentifier)) in self.catalogue):
                print "printObject: reference object number " + sxObjectNumber + " identifier " + itentifier + "value %f" % self.catalogue[(sxObjectNumber, itentifier)]
            else:
                logger.error( "ReferenceCatalogue.printObject: reference Object number " + sxObjectNumber + " for >" + itentifier + "< not found, break")
                break
        else:
#                logger.error( "ReferenceCatalogue.printObject: for object number " + sxObjectNumber + " all elements printed")
            return True

        return False

    def removeSXObject(self, sxObjectNumber):
        if( not sxObjectNumber in self.sxObjects):
            logger.error( "ReferenceCatalogue.removeSXObject: reference Object number " + sxObjectNumber + " for >" + itentifier + "< not found in sxObjects")
        else:
            if( sxObjectNumber in self.sxObjects):
                del self.sxObjects[sxObjectNumber]
            else:
                logger.error( "ReferenceCatalogue.removeSXObject: object number " + sxObjectNumber + " not found")

        for itentifier in self.SExtractorParams.reference:
            if(((sxObjectNumber, itentifier)) in self.catalogue):
                del self.catalogue[(sxObjectNumber, itentifier)]
            else:
                logger.error( "ReferenceCatalogue.removeSXObject: reference Object number " + sxObjectNumber + " for >" + itentifier + "< not found, break")
                break
        else:
#                logger.error( "ReferenceCatalogue.removeSXObject: for object number " + sxObjectNumber + " all elements deleted")
            return True

        return False

    def checkProperties(self, sxObjectNumber): 
        if( self.catalogue[( sxObjectNumber, 'FLAGS')] != 0): # ToDo, ATTENTION
            return False
        elif( self.catalogue[( sxObjectNumber, 'ELLIPTICITY')] > runTimeConfig.value('ELLIPTICITY_REFERENCE')): # ToDo, ATTENTION
            return False
        # TRUE    
        return True

    def checkSeparation( self, position1, position2):
        dx= abs(position2[0]- position1[0])
        dy= abs(position2[1]- position1[1])
        if(( dx < runTimeConfig.value('OBJECT_SEPARATION')) and ( dy< runTimeConfig.value('OBJECT_SEPARATION'))):
            if( (dx**2 + dy**2) < self.objectSeparation2):
                return False
        # TRUE    
        return True

    def checkAcceptance(self, sxObject=None, circle=None):
        distance= sqrt(( float(sxObject.position[0])- circle.transformedCenterX)**2 +(float(sxObject.position[1])- circle.transformedCenterX)**2)
        
        if( circle.transformedRadius >= 0):
            if( distance < abs( circle.transformedRadius)):
                return True
            else:
                return False
        else:
            if( distance < abs( circle.transformedRadius)):
                return False
            else:
                return True

    def cleanUpReference(self):
        if( not  self.isReference):
            logger.error( 'ReferenceCatalogue.cleanUpReference: clean up only for a reference catalogue, I am : ' + self.fitsHDU.fitsFileName) 
            return False
        else:
            logger.error( 'ReferenceCatalogue.cleanUpReference: reference catalogue, I am : ' + self.fitsHDU.fitsFileName)
        flaggedSeparation= 0
        flaggedProperties= 0
        flaggedAcceptance= 0
        for sxObjectNumber1, sxObject1 in self.sxObjects.iteritems():
            for sxObjectNumber2, sxObject2 in self.sxObjects.iteritems():
                if( sxObjectNumber1 != sxObjectNumber2):
                    if( not self.checkSeparation( sxObject1.position, sxObject2.position)):
                        sxObject1.separationOK=False
                        sxObject2.separationOK=False
                        flaggedSeparation += 1

            else:
                if( not self.checkProperties(sxObjectNumber1)):
                    sxObject1.propertiesOK=False
                    flaggedProperties += 1
                if(not self.checkAcceptance(sxObject1, self.circle)):
                    sxObject1.acceptanceOK=False
                    flaggedAcceptance += 1

        discardedObjects= 0
        sxObjectNumbers= self.sxObjects.keys()
        for sxObjectNumber in sxObjectNumbers:
            if(( not self.sxObjects[sxObjectNumber].separationOK) or ( not self.sxObjects[sxObjectNumber].propertiesOK) or ( not self.sxObjects[sxObjectNumber].acceptanceOK)):
                self.removeSXObject( sxObjectNumber)
                discardedObjects += 1

        logger.error("ReferenceCatalogue.cleanUpReference: Number of objects discarded %d  (%d, %d, %d)" % (discardedObjects, flaggedSeparation, flaggedProperties, flaggedAcceptance)) 



class AcceptanceRegion():
    """Class holding the properties of the acceptance circle"""
    def __init__(self, fitsHDU=None, centerOffsetX=None, centerOffsetY=None, radius=None):
        self.fitsHDU= fitsHDU

        binning = fitsHDU.headerElements['BINNING']
        if binning == '1x1':
            self.binning = 1
        elif binning == '2x2':
            self.binning = 2
        elif binning == '4x4':
            self.binning = 4
        else:
            raise Exception('Invalid binning: %s' % (binning))

        self.naxis1 = float(fitsHDU.headerElements['NAXIS1'])
        self.naxis2 = float(fitsHDU.headerElements['NAXIS2'])

        if( centerOffsetX==None):
            self.centerOffsetX= float(runTimeConfig.value('CENTER_OFFSET_X'))
        if( centerOffsetY==None):
            self.centerOffsetY= float(runTimeConfig.value('CENTER_OFFSET_Y'))
        if( radius==None):
            self.radius = float(runTimeConfig.value('RADIUS'))
        l_x= 0. # window (ev.)
        l_y= 0.
        self.transformedCenterX= (self.naxis1- l_x)/2 + self.centerOffsetX 
        self.transformedCenterY= (self.naxis2- l_y)/2 + self.centerOffsetY
        self.transformedRadius= self.radius/ self.binning
        if( verbose):
            print "AcceptanceRegion %f %f %f %f %f %f  %f %f %f" % (self.binning, self.naxis1, self.naxis1, self.centerOffsetX, self.centerOffsetY, self.radius, self.transformedCenterX, self.transformedCenterY, self.transformedRadius)


import numpy
from collections import defaultdict
class Catalogues():
    """Class holding Catalogues"""
    def __init__(self,referenceCatalogue=None):
        self.CataloguesList= []
        self.isValid= False
        self.averageFwhm= {}
        self.averageFlux={}
        self.referenceCatalogue= referenceCatalogue
        if( self.referenceCatalogue != None):
            self.dataFileNameFwhm= serviceFileOp.expandToFitInput( self.referenceCatalogue.fitsHDU, 'FWHM_IMAGE')
            self.dataFileNameFlux= serviceFileOp.expandToFitInput( self.referenceCatalogue.fitsHDU, 'FLUX_MAX')
            self.imageFilename= serviceFileOp.expandToFitImage( self.referenceCatalogue.fitsHDU)
        self.maxFwhm= 0.
        self.minFwhm= 0.
        self.maxFlux= 0.
        self.minFlux= 0.
        self.numberOfObjects= 0.
        self.numberOfObjectsFoundInAllFiles= 0.

    def validate(self):
        if( len(self.CataloguesList) > 0):
            # default is False
            for cat in self.CataloguesList:
                if( cat.isValid== True):
                    if( verbose):
                        print 'Catalogues.validate: valid cat for: ' + cat.fitsHDU.fitsFileName
                    continue
                else:
                    if( verbose):
                        print 'Catalogues.validate: removed cat for: ' + cat.fitsHDU.fitsFileName

                    try:
                        del self.CataloguesList[cat]
                    except:
                        logger.error('Catalogues.validate: could not remove cat for ' + cat.fitsHDU.fitsFileName)
                        if(verbose):
                            print "Catalogues.validate: failed to removed cat: " + cat.fitsHDU.fitsFileName
                    break
            else:
                # still catalogues here
                if( len(self.CataloguesList) > 0):
                    self.isValid= True
                    return self.isValid
                else:
                    logger.error('Catalogues.validate: no catalogues found after validation')
        else:
            logger.error('Catalogues.validate: no catalogues found')
        return False

    def countObjectsFoundInAllFiles(self):
        self.numberOfObjectsFoundInAllFiles= 0
        for sxReferenceObjectNumber, sxReferenceObject in self.referenceCatalogue.sxObjects.items():
            if(sxReferenceObject.numberOfMatches== len(self.CataloguesList)):
                self.numberOfObjectsFoundInAllFiles += 1


    def writeFitInputValues(self):
        self.countObjectsFoundInAllFiles()

        if( self.numberOfObjectsFoundInAllFiles < runTimeConfig.value('MINIMUM_OBJECTS')):
            logger.error('Catalogues.writeFitInputValues: too few sxObjects %d < %d' % (self.numberOfObjectsFoundInAllFiles, runTimeConfig.value('MINIMUM_OBJECTS')))
            return False
        else:

            fitInput= open( self.dataFileNameFwhm, 'wb')
            for focPos in sorted(self.averageFwhm):
                line= "%04d %f\n" % ( focPos, self.averageFwhm[focPos])
                fitInput.write(line)

            fitInput.close()
            fitInput= open( self.dataFileNameFlux, 'wb')
            for focPos in sorted(self.averageFlux):
                line= "%04d %f\n" % ( focPos, self.averageFlux[focPos]/self.maxFlux * self.maxFwhm)
                fitInput.write(line)

            fitInput.close()

        return True

    def fitTheValues(self):
        self.__average__()
        if( self.writeFitInputValues()):
# ROOT, "$fitprg $fltr $date $number_of_objects_found_in_all_files $fwhm_file $flux_file $tmp_fit_result_file
            cmd= [ runTimeConfig.value('FOCROOT'),
                   "1", # 1 interactive, 0 batch
                   self.referenceCatalogue.fitsHDU.headerElements['FILTER'],
                   serviceFileOp.now,
                   str(self.numberOfObjectsFoundInAllFiles),
                   self.dataFileNameFwhm,
                   self.dataFileNameFlux,
                   self.imageFilename]
        
            output = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
            print output
        else:
            logger.error('Catalogues.fitTheValues: do not fit')

    def printSelectedSXobjects(self):
        for sxReferenceObjectNumber, sxReferenceObject in self.referenceCatalogue.sxObjects.items():
            if( sxReferenceObject.foundInAll): 
                print "======== %d %d %f %f" %  (int(sxReferenceObjectNumber), sxReferenceObject.focusPosition, sxReferenceObject.position[0],sxReferenceObject.position[1])

    def __average__(self):
        fwhm= defaultdict(list)
        flux= defaultdict(list)
        lenFwhm = []
        for sxReferenceObjectNumber, sxReferenceObject in self.referenceCatalogue.sxObjects.items():
            if(sxReferenceObject.numberOfMatches== len(self.CataloguesList)):
                self.numberOfObjects += 1
                for sxObject in sxReferenceObject.matchedsxObjects:
             #       if( sxObject.focusPosition > 2600):
                    #if( verbose):
                    #    print "Ref "+ sxReferenceObject.objectNumber + " Obj "+ sxObject.objectNumber + " foc pos %d" % sxObject.focusPosition     
                    fwhm[sxObject.focusPosition].append(sxObject.fwhm)
                    flux[sxObject.focusPosition].append(sxObject.flux)
                    sxReferenceObject.foundInAll= True
                lenFwhm.append(len(fwhm))

        fwhmList=[]
        fluxList=[]
        for focPos in fwhm:
            self.averageFwhm[focPos] = numpy.mean(fwhm[focPos], axis=0)
            fwhmList.append(self.averageFwhm[focPos])
            
        for focPos in flux:
            self.averageFlux[focPos] = numpy.mean(flux[focPos], axis=0)
            fluxList.append(self.averageFlux[focPos])

        self.maxFwhm= numpy.amax(fwhmList)
        self.minFwhm= numpy.amax(fwhmList)
        self.maxFlux= numpy.amax(fluxList)
        self.minFlux= numpy.amax(fluxList)
        if(verbose):
            print "numberOfObjects========================= %d " % (self.numberOfObjects)

        for focPos in sorted(fwhm):
            print "average %d %f %f" % (focPos, self.averageFwhm[focPos], self.averageFlux[focPos])

import sys
import pyfits

class FitsHDU():
    """Class holding fits file name and its properties"""
    def __init__(self, fitsFileName=None, referenceFitsHDU=None):
        if(referenceFitsHDU==None):
            self.fitsFileName= serviceFileOp.expandToRunTimePath(fitsFileName)
        else:
            self.fitsFileName= fitsFileName

        self.referenceFitsHDU= referenceFitsHDU
        self.isValid= False
        self.headerElements={}
        FitsHDU.__lt__ = lambda self, other: self.headerElements['FOC_POS'] < other.headerElements['FOC_POS']


    def headerProperties(self):
        if( verbose):
            print "fits file path: " + self.fitsFileName
        try:
            fitsHDU = pyfits.fitsopen(self.fitsFileName)
        except:
            if( self.fitsFileName):
                logger.error('FitsHDU: file not found: %s' % self.fitsFileName)
            else:
                logger.error('FitsHDU: file name not given (None)')
            return False

        fitsHDU.close()
        if( self.referenceFitsHDU== None):
            if(verbose):
                print "headerProperties I'm compatible with myself"

            try:
                self.headerElements['FILTER'] = fitsHDU[0].header['FILTER']
                self.headerElements['FOC_POS']= fitsHDU[0].header['FOC_POS']
                self.headerElements['BINNING']= fitsHDU[0].header['BINNING']
                self.headerElements['NAXIS1'] = fitsHDU[0].header['NAXIS1']
                self.headerElements['NAXIS2'] = fitsHDU[0].header['NAXIS2']
                self.headerElements['ORIRA']  = fitsHDU[0].header['ORIRA']
                self.headerElements['ORIDEC'] = fitsHDU[0].header['ORIDEC']
                self.isValid= True
                return True
            except:
                logger.error('FitsHDU: the required header element not found, exiting')
                sys.exit(1)
                return False

        keys= self.referenceFitsHDU.headerElements.keys()
        i= 0
        for key in keys:
            if(key == 'FOC_POS'):
                self.headerElements['FOC_POS']= fitsHDU[0].header['FOC_POS']
                continue
            if(self.referenceFitsHDU.headerElements[key]!= fitsHDU[0].header[key]):
                logger.error("headerProperties: fits file " + self.fitsFileName + " property " + key + " " + self.referenceFitsHDU.headerElements[key] + " " + fitsHDU[0].header[key])
                break
            else:
                self.headerElements[key] = fitsHDU[0].header[key]

        else: # exhausted
            self.isValid= True
            if( verbose):
                print 'headerProperties: header ok : ' + self.fitsFileName

            return True
        
        logger.error("headerProperties: fits file " + self.fitsFileName + " has different header properties")
        return False

class FitsHDUs():
    """Class holding FitsHDU"""
    def __init__(self):
        self.fitsHDUsList= []
        self.isValid= False
        self.numberOfFocusPositions= 0 

    def findHDUByFitsFileName( self, fileName):
        for hdu in sorted(self.fitsHDUsList):
            if( fileName== hdu.fitsFileName):
                return hdu
        return False

    def findReference(self):
        for hdu in self.fitsHDUsList:
            if( hdu.referenceFitsHDU== None):
                if(verbose):
                    print "FitsHDUs.findReference: FOUND------------------HDU "+ hdu.fitsFileName
                return hdu
        return False

    def findReferenceByFocPos(self, filterName):
        filter=  runTimeConfig.filterByName(filterName)
#        for hdu in sorted(self.fitsHDUsList):
        for hdu in sorted(self.fitsHDUsList):
            if( filter.focDef < hdu.headerElements['FOC_POS']):
                if(verbose):
                    print "FOUND reference by foc pos ==============" + hdu.fitsFileName + "======== %d" % hdu.headerElements['FOC_POS']
                return hdu
        return False

    def validate(self, filterName=None):
        hdur= self.findReference()
        if( hdur== False):
            hdur= self.findReferenceByFocPos(filterName)
            if( hdur== False):
                if( verbose):
                    print "Nothing found in findReference and findReferenceByFocPos"
                return self.isValid

        keys= hdur.headerElements.keys()
        i= 0
        differentFocuserPositions={}
        for hdu in self.fitsHDUsList:
            for key in keys:
                if(key == 'FOC_POS'):
                    differentFocuserPositions[str(hdu.headerElements['FOC_POS'])]= 1 # means nothing
                    continue
                if( hdur.headerElements[key]!= hdu.headerElements[key]):
                    try:
                        del self.fitsHDUsList[hdu]
                    except:
                        logger.error('FitsHDUs.validate: could not remove hdu for ' + hdu.fitsFileName)
                        if(verbose):
                            print "FitsHDUs.validate: removed hdu: " + hdu.fitsFileName + "========== %d" %  self.fitsHDUsList.index(hdu)
                    break # really break out the keys loop 
            else: # exhausted
                hdu.isValid= True
                if( verbose):
                    print 'FitsHDUs.validate: valid hdu: ' + hdu.fitsFileName

        self.numberOfFocusPositions= len(differentFocuserPositions.keys())

        if( self.numberOfFocusPositions >= runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')):
            if( len(self.fitsHDUsList) > 0):
                logger.error('FitsHDUs.validate: hdus are valid')
                self.isValid= True
        else:
            logger.error('FitsHDUs.validate: hdus are invalid due too few focuser positions %d required are %d' % (self.numberOfFocusPositions, runTimeConfig.value('MINIMUM_FOCUSER_POSITIONS')))
        logger.error('FitsHDUs.validate: hdus are invalid')
        return self.isValid
    def countFocuserPositions(self):
        differentFocuserPositions={}
        for hdu in self.fitsHDUsList:
            differentFocuserPositions(hdu.headerElements['FOC_POS'])
            
        
        return len(differentFocuserPositions)

import time
import datetime
import glob
import os

class ServiceFileOperations():
    """Class performing various task on files, e.g. expansion to (full) path"""
    def __init__(self):
        self.now= datetime.datetime.now().isoformat()
        self.runTimePath='/'

    def prefix( self, fileName):
        return 'rts2af-' +fileName

    def notFits(self, fileName):
        items= fileName.split('/')[-1]
        return items.split('.fits')[0]

    def absolutePath(self, fileName=None):
        if( fileName==None):
            logger.error('ServiceFileOperations.absolutePath: no file name given')

        pLeadingSlash = re.compile( r'\/.*')
        leadingSlash = pLeadingSlash.match(fileName)
        if( leadingSlash):
            return True
        else:
            return False

    def fitsFilesInRunTimePath( self):
        if( verbose):
            print "searching in " + repr(glob.glob( self.runTimePath + '/' + runTimeConfig.value('FILE_GLOB')))
        return glob.glob( self.runTimePath + '/' + runTimeConfig.value('FILE_GLOB'))

    def expandToTmp( self, fileName=None):
        if( self.absolutePath(fileName)):
            return fileName

        fileName= runTimeConfig.value('TEMP_DIRECTORY') + '/'+ fileName
        return fileName

    def expandToSkyList( self, fitsHDU=None):
        if( fitsHDU==None):
            logger.error('ServiceFileOperations.expandToCat: no hdu given')
        
        items= runTimeConfig.value('SEXSKY_LIST').split('.')
        try:
            fileName= self.prefix( items[0] +  '-' + fitsHDU.headerElements['FILTER'] + '-' +   self.now + '.' + items[1])
        except:
            fileName= self.prefix( items[0] + '-' +   self.now + '.' + items[1])
            
        if(verbose):
            print 'ServiceFileOperations:expandToSkyList expanded to ' + fileName
        
        return  self.expandToTmp(fileName)

    def expandToCat( self, fitsHDU=None):
        if( fitsHDU==None):
            logger.error('ServiceFileOperations.expandToCat: no hdu given')
        try:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + fitsHDU.headerElements['FILTER'] + '-' + self.now + '.cat')
        except:
            fileName= self.prefix( self.notFits(fitsHDU.fitsFileName) + '-' + self.now + '.cat')

        return self.expandToTmp(fileName)

    def expandToFitInput(self, fitsHDU=None, element=None):
        items= runTimeConfig.value('FIT_RESULT_FILE').split('.')
        if((fitsHDU==None) or ( element==None)):
            logger.error('ServiceFileOperations.expandToFitInput: no hdu or elementgiven')

        fileName= items[0] + "-" + fitsHDU.headerElements['FILTER'] + "-" + self.now +  "-" + element + "." + items[1]
        return self.expandToTmp(self.prefix(fileName))

    def expandToFitImage(self, fitsHDU=None):
        if( fitsHDU==None):
            logger.error('ServiceFileOperations.expandToFitImage: no hdu given')

        items= runTimeConfig.value('FIT_RESULT_FILE').split('.') 
# ToDo png
        fileName= items[0] + "-" + fitsHDU.headerElements['FILTER'] + "-" + self.now + ".png"
        return self.expandToTmp(self.prefix(fileName))

    def expandToRunTimePath(self, fileName=None):
        if( self.absolutePath(fileName)):
            return fileName
        else:
            return self.runTimePath + '/' + fileName

    def defineRunTimePath(self, fileName=None):
        if( self.absolutePath(fileName)):
            return fileName

        for root, dirs, names in os.walk(runTimeConfig.value('BASE_DIRECTORY')):
            if( fileName in names):
                self.runTimePath= root
                return True
        return False


#
# stub, will be called in main script 
runTimeConfig= Configuration()
serviceFileOp= ServiceFileOperations()
verbose= False
class Service():
    """Temporary class for uncategorized methods"""
    def __init__(self):
        print

#  LocalWords:  itentifier
