#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QLineF>
#include "common/config.h"
#include "gpxparser.h"
#include "tcxparser.h"
#include "csvparser.h"
#include "kmlparser.h"
#include "fitparser.h"
#include "igcparser.h"
#include "nmeaparser.h"
#include "oziparsers.h"
#include "locparser.h"
#include "slfparser.h"
#ifdef ENABLE_GEOJSON
#include "geojsonparser.h"
#endif // ENABLE_GEOJSON
#include "exifparser.h"
#include "cupparser.h"
#include "gpiparser.h"
#include "smlparser.h"
#include "dem.h"
#include "data.h"


static GPXParser gpx;
static TCXParser tcx;
static KMLParser kml;
static FITParser fit;
static CSVParser csv;
static IGCParser igc;
static NMEAParser nmea;
static PLTParser plt;
static WPTParser wpt;
static RTEParser rte;
static LOCParser loc;
static SLFParser slf;
#ifdef ENABLE_GEOJSON
static GeoJSONParser geojson;
#endif // ENABLE_GEOJSON
static EXIFParser exif;
static CUPParser cup;
static GPIParser gpi;
static SMLParser sml;

static QMap<QString, Parser*> parsers()
{
	QMap<QString, Parser*> map;

	map.insert("gpx", &gpx);
	map.insert("tcx", &tcx);
	map.insert("kml", &kml);
	map.insert("fit", &fit);
	map.insert("csv", &csv);
	map.insert("igc", &igc);
	map.insert("nmea", &nmea);
	map.insert("plt", &plt);
	map.insert("wpt", &wpt);
	map.insert("rte", &rte);
	map.insert("loc", &loc);
	map.insert("slf", &slf);
#ifdef ENABLE_GEOJSON
	map.insert("json", &geojson);
	map.insert("geojson", &geojson);
#endif // ENABLE_GEOJSON
	map.insert("jpeg", &exif);
	map.insert("jpg", &exif);
	map.insert("cup", &cup);
	map.insert("gpi", &gpi);
	map.insert("sml", &sml);

	return map;
}


QMap<QString, Parser*> Data::_parsers = parsers();
bool Data::_useDEM = false;

void Data::processData(QList<TrackData> &trackData, QList<RouteData> &routeData)
{
	for (int i = 0; i < trackData.count(); i++) {
		TrackData &track = trackData[i];
		for (int j = 0; j < track.size(); j++) {
			SegmentData &segment = track[j];
			for (int k = 0; k < segment.size(); k++) {
				Trackpoint &t = segment[k];
				if (!t.hasElevation() || _useDEM) {
					qreal elevation = DEM::elevation(t.coordinates());
					if (!std::isnan(elevation))
						t.setElevation(elevation);
				}
			}
		}
		_tracks.append(Track(trackData.at(i)));
	}
	for (int i = 0; i < routeData.count(); i++) {
		RouteData &route = routeData[i];
		for (int j = 0; j < route.size(); j++) {
			Waypoint &w = route[j];
			if (!w.hasElevation() || _useDEM) {
				qreal elevation = DEM::elevation(w.coordinates());
				if (!std::isnan(elevation))
					w.setElevation(elevation);
			}
		}
		_routes.append(Route(routeData.at(i)));
	}
	for (int i = 0; i < _waypoints.size(); i++) {
		if (!_waypoints.at(i).hasElevation() || _useDEM) {
			qreal elevation = DEM::elevation(_waypoints.at(i).coordinates());
			if (!std::isnan(elevation))
				_waypoints[i].setElevation(elevation);
		}
	}
}

Data::Data(const QString &fileName, bool poi)
{
	QFile file(fileName);
	QFileInfo fi(fileName);
	QList<TrackData> trackData;
	QList<RouteData> routeData;

	_valid = false;
	_errorLine = 0;

	if (!file.open(QFile::ReadOnly)) {
		_errorString = qPrintable(file.errorString());
		return;
	}

	QMap<QString, Parser*>::iterator it;
	if ((it = _parsers.find(fi.suffix().toLower())) != _parsers.end()) {
		if (it.value()->parse(&file, trackData, routeData, _polygons,
		  _waypoints)) {
			if (!poi)
				processData(trackData, routeData);
			_valid = true;
			return;
		} else {
			_errorLine = it.value()->errorLine();
			_errorString = it.value()->errorString();
		}
	} else {
		for (it = _parsers.begin(); it != _parsers.end(); it++) {
			if (it.value()->parse(&file, trackData, routeData, _polygons,
			  _waypoints)) {
				if (!poi)
					processData(trackData, routeData);
				_valid = true;
				return;
			}
			file.reset();
		}

		qWarning("Error loading data file: %s:", qPrintable(fileName));
		for (it = _parsers.begin(); it != _parsers.end(); it++)
			qWarning("%s: line %d: %s", qPrintable(it.key()),
			  it.value()->errorLine(), qPrintable(it.value()->errorString()));

		_errorLine = 0;
		_errorString = "Unknown format";
	}
}

QString Data::formats()
{
	return
	  qApp->translate("Data", "Supported files") + " (" + filter().join(" ") + ");;"
	  + qApp->translate("Data", "CSV files") + " (*.csv);;"
	  + qApp->translate("Data", "CUP files") + " (*.cup);;"
	  + qApp->translate("Data", "FIT files") + " (*.fit);;"
#ifdef ENABLE_GEOJSON
	  + qApp->translate("Data", "GeoJSON files") + " (*.geojson *.json);;"
#endif // ENABLE_GEOJSON
	  + qApp->translate("Data", "GPI files") + " (*.gpi);;"
	  + qApp->translate("Data", "GPX files") + " (*.gpx);;"
	  + qApp->translate("Data", "IGC files") + " (*.igc);;"
	  + qApp->translate("Data", "JPEG images") + " (*.jpg *.jpeg);;"
	  + qApp->translate("Data", "KML files") + " (*.kml);;"
	  + qApp->translate("Data", "LOC files") + " (*.loc);;"
	  + qApp->translate("Data", "NMEA files") + " (*.nmea);;"
	  + qApp->translate("Data", "OziExplorer files") + " (*.plt *.rte *.wpt);;"
	  + qApp->translate("Data", "SLF files") + " (*.slf);;"
	  + qApp->translate("Data", "SML files") + " (*.sml);;"
	  + qApp->translate("Data", "TCX files") + " (*.tcx);;"
	  + qApp->translate("Data", "All files") + " (*)";
}

QStringList Data::filter()
{
	QStringList filter;

	for (QMap<QString, Parser*>::iterator it = _parsers.begin();
	  it != _parsers.end(); it++)
		filter << "*." + it.key();

	return filter;
}

void Data::useDEM(bool use)
{
	_useDEM = use;
}
