# (C) 2013, Markus Wildi, markus.wildi@bluewin.ch
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

import unittest

from rts2saf.config import Configuration 
from rts2saf.devices import CCD, Focuser, FilterWheel, Filter

import logging
logging.basicConfig(filename='/tmp/unittest.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
logger = logging.getLogger()

# sequence matters
def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestConfiguration('test_readConfiguration'))
    suite.addTest(TestConfiguration('test_checkConfiguration'))

    return suite

#@unittest.skip('class not yet implemented')
class TestConfiguration(unittest.TestCase):

    def tearDown(self):
        pass

    def setUp(self):
        self.rt = Configuration(logger=logger)
        self.fileName='/usr/local/etc/rts2/rts2saf/rts2saf.cfg'
        self.success=self.rt.readConfiguration(fileName=self.fileName)

    #@unittest.skip('feature not yet implemented')
    def test_readConfiguration(self):
        self.assertTrue(self.success, 'config file: {} faulty or not found'.format(self.fileName))

        self.assertIs(type(self.rt), Configuration)
        self.assertIs(type(self.rt.ccd), CCD)
        self.assertIs(type(self.rt.foc), Focuser)
        self.assertIs(type(self.rt.filterWheels[0]), FilterWheel)
        self.assertIs(type(self.rt.filterWheels[0].filters[0]), Filter)


    def test_checkConfiguration(self):
        self.assertTrue(self.success, 'config file: {} faulty or not found'.format(self.fileName))
        result = self.rt.checkConfiguration()
        self.assertTrue(result)

if __name__ == '__main__':
    
#    suite = unittest.TestLoader().loadTestsFromTestCase(TestFitFwhm)
    unittest.TextTestRunner(verbosity=0).run(suite())
