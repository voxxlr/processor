#include "laszip/laszip_api.h"

#include "las.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

LasImporter::LasImporter(json_spirit::mObject& iConfig)
: CloudImporter(iConfig)
{
};

struct GeoKeys
{
	unsigned short wKeyDirectoryVersion;
	unsigned short wKeyRevision;
	unsigned short wMinorRevision;
	unsigned short wNumberOfKeys;
	struct Entry
	{
		unsigned short wKeyID;
		unsigned short wTIFFTagLocation;
		unsigned short wCount;
		unsigned short wValue_Offset;
	} 
	pKey[8]; 
};

json_spirit::mObject LasImporter::import (std::string iName)
{
	laszip_POINTER lReader;
	laszip_header* lHeader;
	laszip_point* lLazPoint;

	laszip_create(&lReader);
	laszip_BOOL request_reader = 1;
	laszip_request_compatibility_mode(lReader, request_reader);
	laszip_BOOL is_compressed = true;
	laszip_open_reader(lReader, iName.c_str(), &is_compressed);
	laszip_get_header_pointer(lReader, &lHeader);

	laszip_get_point_pointer(lReader, &lLazPoint);
	unsigned long lPointCount = (lHeader->number_of_point_records ? lHeader->number_of_point_records : lHeader->extended_number_of_point_records);
	BOOST_LOG_TRIVIAL(info) << iName << " contains " << lPointCount << " points ";

	PointCloudAttributes lAttributes;
	int lIntensityIndex = lAttributes.createAttribute(Attribute::INTENSITY, INTENSITY_TEMPLATE);
	int lColorIndex = -1;
	if (lHeader->point_data_format > 1)
	{
		lColorIndex = lAttributes.createAttribute(Attribute::COLOR, COLOR_TEMPLATE);
	}
    
	uint16_t minI = std::numeric_limits<uint16_t>::max();
	uint16_t maxI = 0;

    uint8_t minClass = 255;
    uint8_t maxClass = 0;

	// AABB pass to find center
	BOOST_LOG_TRIVIAL(info) << "AABB pass ";
	laszip_seek_point(lReader, 0);
	for(unsigned long long p=0; p < lPointCount; p++)
	{
		laszip_read_point(lReader);
		double lCoords[3];
		lCoords[0] = (lLazPoint->X)*lHeader->x_scale_factor + lHeader->x_offset;
		lCoords[1] = (lLazPoint->Y)*lHeader->y_scale_factor + lHeader->y_offset;
		lCoords[2] = (lLazPoint->Z)*lHeader->z_scale_factor + lHeader->z_offset;
		convertCoords(lCoords);

        minClass = std::min(lLazPoint->classification, minClass);
        maxClass = std::max(lLazPoint->classification, maxClass);
  
		growMinMax(lCoords);

		minI = std::min(lLazPoint->intensity, minI);
		maxI = std::max(lLazPoint->intensity, maxI);

		if (p%10000000==0)
		{
			BOOST_LOG_TRIVIAL(info) << p;
		}

        // REMOVE_
        // if ((p+1)%10000000==0)
		// {
        //      break;
        //  }
        // REMOVE_
	}

	glm::dvec3 lCenter;
	lCenter[0] = (mMinD[0] + mMaxD[0]) / 2;
	lCenter[1] = (mMinD[1] + mMaxD[1]) / 2;
	lCenter[2] = (mMinD[2] + mMaxD[2]) / 2;

	mMinD[0] -= lCenter[0];
	mMinD[1] -= lCenter[1];
	mMinD[2] -= lCenter[2];
	mMaxD[0] -= lCenter[0];
	mMaxD[1] -= lCenter[1];
	mMaxD[2] -= lCenter[2];


    int lClassIndex = -1;
    if (minClass != maxClass)
    {
        lClassIndex = lAttributes.createAttribute(Attribute::CLASS, CLASS_TEMPLATE);
    }
	double lScalerI = std::numeric_limits<uint16_t>::max()/(double)(maxI - minI != 0 ? maxI - minI : 1);


    //
    // Main Pass
    //

	Point lPoint(lAttributes);
	IntensityType& lIntensity = *(IntensityType*)lPoint.getAttribute(lIntensityIndex);

    ClassType* lClass = 0;
    if (lClassIndex!= -1)
    {
	    lClass = (ClassType*)lPoint.getAttribute(lClassIndex);
    }

	ColorType* lColor = 0;
	if (lHeader->point_data_format > 1)
	{
		lColor = (ColorType*)lPoint.getAttribute(lColorIndex);
	}

	/*
	for (int i=0; i<std::max(iMax, 18); i++)
	{
		//iCloud.addClass(sDefaultClasses[i*3+0], sDefaultClasses[i*3+1], sDefaultClasses[i*3+2]);  
	}
	*/
	boost::filesystem::path lPath(iName);

	// main pass
	BOOST_LOG_TRIVIAL(info) << "Main pass ";
	FILE* lOutputFile = PointCloud::writeHeader(lPath.stem().string(), lAttributes);
	laszip_seek_point(lReader, 0);
	for(unsigned long p=0; p < lPointCount; p++)
	{
		laszip_read_point(lReader);
		double lCoords[3];
		lCoords[0] = (lLazPoint->X)*lHeader->x_scale_factor + lHeader->x_offset;
		lCoords[1] = (lLazPoint->Y)*lHeader->y_scale_factor + lHeader->y_offset;
		lCoords[2] = (lLazPoint->Z)*lHeader->z_scale_factor + lHeader->z_offset;
		convertCoords(lCoords);
		lCoords[0] -= lCenter[0];
		lCoords[1] -= lCenter[1];
		lCoords[2] -= lCenter[2];
		growMinMax(lCoords);

		lPoint.position[0] = (float)lCoords[0];
		lPoint.position[1] = (float)lCoords[1];
		lPoint.position[2] = (float)lCoords[2];

		lIntensity.mValue = lLazPoint->intensity*lScalerI;
        if(lClass)
        {
		    lClass->mValue = lLazPoint->classification; 
        }

		if (lColor)
		{
			laszip_U16 r = lLazPoint->rgb[0];
			laszip_U16 g = lLazPoint->rgb[1];
			laszip_U16 b = lLazPoint->rgb[2];

			if (r > 255 || g > 255 || b > 255)
			{
				lColor->mValue[0] = r/256;
				lColor->mValue[1] = g/256;
				lColor->mValue[2] = b/256;
			}
			else
			{
				lColor->mValue[0] = (uint8_t)r;
				lColor->mValue[1] = (uint8_t)g;
				lColor->mValue[2] = (uint8_t)b;
			}
		}
		lPoint.write(lOutputFile);
		
		if (p%10000000==0)
		{
			BOOST_LOG_TRIVIAL(info) << p;
		}

        // REMOVE_
       // if ((p+1)%10000000==0)
		//{
       //     lPointCount = p;
       //     break;
       // }
        // REMOVE_
	}

	PointCloud::updateSize(lOutputFile, lPointCount);
	PointCloud::updateSpatialBounds(lOutputFile, mMinD, mMaxD);
	fclose(lOutputFile);
	laszip_close_reader(lReader);

	json_spirit::mObject lMeta = getMeta();
	lMeta["file"] = lPath.stem().string();
	return lMeta;
}

/*
uint64_t LasImporter::append(FILE* iFile, std::string iName, PointCloud& iCloud)
{
	laszip_POINTER lReader;
	laszip_header* lHeader;
	laszip_point* lLazPoint;

	laszip_create(&lReader);
	laszip_BOOL request_reader = 1;
	laszip_request_compatibility_mode(lReader, request_reader);
	laszip_BOOL is_compressed = true;
	laszip_open_reader(lReader, iName.c_str(), &is_compressed);
	laszip_get_header_pointer(lReader, &lHeader);
	laszip_get_point_pointer(lReader, &lLazPoint);

	unsigned long lPointCount = (lHeader->number_of_point_records ? lHeader->number_of_point_records : lHeader->extended_number_of_point_records);
	BOOST_LOG_TRIVIAL(info) << iName << " contains " << lPointCount << " points ";

	// write ply
	Point lPoint(iCloud);
	IntensityType& Intensity = *(IntensityType*)lPoint.getAttribute(iCloud.getAttributeIndex(Attribute::INTENSITY));
	ClassType& lClass = *(ClassType*)lPoint.getAttribute(iCloud.getAttributeIndex(Attribute::CLASS));
	ColorType* lColor = (ColorType*)lPoint.getAttribute(iCloud.getAttributeIndex(Attribute::COLOR));

	// AABB pass
	laszip_seek_point(lReader, 0);
	for(unsigned long p=0; p < lPointCount; p++)
	{
		laszip_read_point(lReader);
		lPoint.position[0] = (lLazPoint->X)*lHeader->x_scale_factor + lHeader->x_offset - 430050.04525049235; // JSTIER REMOVE THIS!!!
		lPoint.position[1] = (lLazPoint->Y)*lHeader->y_scale_factor + lHeader->y_offset - 4583052.3342177588;// JSTIER REMOVE THIS!!!
		lPoint.position[2] = (lLazPoint->Z)*lHeader->z_scale_factor + lHeader->z_offset;
		convertCoords(lPoint);

		Intensity.mValue = lLazPoint->intensity;
		lClass.mValue = lLazPoint->classification; 
		if (lHeader->point_data_format > 1)
		{
			laszip_U16 r = lLazPoint->rgb[0];
			laszip_U16 g = lLazPoint->rgb[1];
			laszip_U16 b = lLazPoint->rgb[2];

			if (r > 255 || g > 255 || b > 255)
			{
				lColor->mValue[0] = r/256;
				lColor->mValue[1] = g/256;
				lColor->mValue[2] = b/256;
			}
			else
			{
				lColor->mValue[0] = r;
				lColor->mValue[1] = g;
				lColor->mValue[2] = b;
			}
		}
		
		lPoint.write(iFile);
	}

	laszip_close_reader(lReader);
	return lPointCount;
}

*/



	/*
	FILE* file = fopen(file_name, "rb+");

if (set_geotiff_vlr_geo_keys_pos != -1)
          {
            GeoProjectionConverter geo;
            if (geo.set_epsg_code(set_geotiff_epsg))
            {
              int number_of_keys;
              GeoProjectionGeoKeys* geo_keys = 0;
              int num_geo_double_params;
              double* geo_double_params = 0;
              if (geo.get_geo_keys_from_projection(number_of_keys, &geo_keys, num_geo_double_params, &geo_double_params))
              {
                U32 set_geotiff_vlr_geo_keys_new_length = sizeof(GeoProjectionGeoKeys)*(number_of_keys+1);

                if (set_geotiff_vlr_geo_keys_new_length <= set_geotiff_vlr_geo_keys_length)
                {
                  fseek(file,(long)set_geotiff_vlr_geo_keys_pos, SEEK_SET);
                  LASvlr_geo_keys vlr_geo_key_directory;
                  vlr_geo_key_directory.key_directory_version = 1;
                  vlr_geo_key_directory.key_revision = 1;
                  vlr_geo_key_directory.minor_revision = 0;
                  vlr_geo_key_directory.number_of_keys = number_of_keys;
                  fwrite(&vlr_geo_key_directory, sizeof(LASvlr_geo_keys), 1, file);
                  fwrite(geo_keys, sizeof(GeoProjectionGeoKeys), number_of_keys, file);
                  for (i = set_geotiff_vlr_geo_keys_new_length; i < (int)set_geotiff_vlr_geo_keys_length; i++)
                  {
                    fputc(0, file);
                  }

                  if (set_geotiff_vlr_geo_double_pos != -1)
                  {
                    fseek(file,(long)set_geotiff_vlr_geo_double_pos, SEEK_SET);
                    for (i = 0; i < (int)set_geotiff_vlr_geo_double_length; i++)
                    {
                      fputc(0, file);
                    }
                  }

                  if (set_geotiff_vlr_geo_ascii_pos != -1)
                  {
                    fseek(file,(long)set_geotiff_vlr_geo_ascii_pos, SEEK_SET);
                    for (i = 0; i < (int)set_geotiff_vlr_geo_ascii_length; i++)
                    {
                      fputc(0, file);
                    }
                  }
                }
                else
                {
                  fprintf(stderr, "WARNING: cannot set EPSG to %u because file '%s' has not enough header space for GeoTIFF tags\n", set_geotiff_epsg, file_name);
                }
              }
              else
              {
                fprintf(stderr, "WARNING: cannot set EPSG in GeoTIFF tags of because no GeoTIFF tags available for code %u\n", set_geotiff_epsg);
                set_geotiff_epsg = -1;
              }
            }
            else
            {
              fprintf(stderr, "WARNING: cannot set EPSG in GeoTIFF tags of because code %u is unknown\n", set_geotiff_epsg);
              set_geotiff_epsg = -1;
            }
          }
          else
          {
            fprintf(stderr, "WARNING: cannot set EPSG to %u because file '%s' has no GeoTIFF tags\n", set_geotiff_epsg, file_name);
          }
        }
        if (verbose) fprintf(stderr, "edited '%s' ...\n", file_name);
        fclose(file);
		*/


	/*
	long lPos = lHeader->header_size;
	double* lDoubleParams = 0;
	char* lCharParams = 0;
	GeoKeys* lDirectory = 0;
	for (int i=0; i < lHeader->number_of_variable_length_records; i++)
	{
		lPos += 54;
		if (std::string(lHeader->vlrs[i].user_id) == "LASF_Projection")
		{
			if (lHeader->vlrs[i].record_id == 34735) // GeoKeyDirectoryTag
			{
				lDirectory = (GeoKeys*)lHeader->vlrs[i].data;
			}
			else if (lHeader->vlrs[i].record_id == 34736) // GeoDoubleParamsTag
			{
				lDoubleParams = (double*)lHeader->vlrs[i].data;
			}
			else if (lHeader->vlrs[i].record_id == 34737) // GeoAsciiParamsTag
			{
				lCharParams = (char*)lHeader->vlrs[i].data;
			}
		}
		lPos += lHeader->vlrs[i].record_length_after_header;
	}

	if (lDirectory)
	{
		for (int k=0; k<lDirectory->wNumberOfKeys; k++)
		{
			GeoKeys::Entry& lKey = lDirectory->pKey[k];
			std::cout << lKey.wKeyID;
			switch (lKey.wTIFFTagLocation)
			{
			case 0:
				std::cout << " = " << lKey.wValue_Offset <<"\n";
				break;
			case 34736:
				std::cout << " = " << lDoubleParams[lKey.wValue_Offset] <<"\n";
				break;
			case 34737:
				std::cout << " = " << &lCharParams[lKey.wValue_Offset] <<"\n";
				break;
			}
			//std::cout << lKey.wKeyID << "," << lKey.wTIFFTagLocation << "," << lKey.wCount <<"\n";
		}

		// geo referenced assume LEFT_Z_UP
		mCoords = LEFT_Z_UP;
	}
	*/