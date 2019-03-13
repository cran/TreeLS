//  ===============================================================================
//
//  Developers:
//
//  Tiago de Conto - ti@forlidar.com.br -  https://github.com/tiagodc/
//
//  COPYRIGHT: Tiago de Conto, 2019
//
//  This piece of software is open and free to use, redistribution and modifications
//  should be done in accordance to the GNU General Public License >= 3
//
//  Use this software as you wish, but no warranty is provided whatsoever. For any
//  comments or questions on TreeLS, please contact the developer (prefereably through my github account)
//
//  If publishing any work/study/research that used the tools in TreeLS,
//  please don't forget to cite the proper sources!
//
//  Enjoy!
//
//  ===============================================================================

#include "methods.hpp"

// [[Rcpp::plugins("cpp11")]]

using namespace std;

// export tree positions point stack
List exportTreeMap(vector<HoughCenters>& coordinates){

  vector<double> xout;
  vector<double> yout;
  vector<double> zout;
  vector<double> radii;
  vector<bool> keyFlag;
  vector<bool> treeFlag;
  vector<unsigned short int> votes;
  vector<unsigned int> treeId;
  vector<unsigned int> discId;
  // vector<unsigned int> nPoints;


  unsigned int diskCounter = 1;
  unsigned int maxId = 0;

  for(auto& point : coordinates){

    if(point.tree_id == 0)
      continue;

    if(point.tree_id > maxId)
      maxId = point.tree_id;

    double z = (point.low_z + point.up_z)/2;

    // main point
    xout.push_back(point.main_circle.x_center);
    yout.push_back(point.main_circle.y_center);
    zout.push_back(z);

    radii.push_back(point.main_circle.radius);
    votes.push_back(point.main_circle.n_votes);
    treeId.push_back(point.tree_id);
    // nPoints.push_back(point.circles.size());
    discId.push_back(diskCounter);
    keyFlag.push_back(true);
    treeFlag.push_back(false);

    // other candidates
    for(auto& c_point : point.circles){

      xout.push_back(c_point.x_center);
      yout.push_back(c_point.y_center);
      zout.push_back(z);

      radii.push_back(c_point.radius);
      votes.push_back(c_point.n_votes);
      treeId.push_back(point.tree_id);
      // nPoints.push_back(point.circles.size());
      discId.push_back(diskCounter);
      keyFlag.push_back(false);
      treeFlag.push_back(false);
    }
    diskCounter++;
  }

  vector<double> xSums(maxId, 0);
  vector<double> ySums(maxId, 0);
  vector<unsigned int> counters(maxId, 0);

  for(auto& point : coordinates){

    if(point.tree_id == 0)
      continue;

    xSums[point.tree_id-1] += point.main_circle.x_center;
    ySums[point.tree_id-1] += point.main_circle.y_center;
    counters[point.tree_id-1]++;
  }

  for(unsigned int i = 0; i < maxId; ++i){

    if(counters[i] == 0)
      continue;

    double mainX = xSums[i] / counters[i];
    double mainY = ySums[i] / counters[i];

    xout.push_back(mainX);
    yout.push_back(mainY);
    zout.push_back(0);

    radii.push_back(0);
    votes.push_back(0);
    treeId.push_back(i+1);
    // nPoints.push_back(0);
    discId.push_back(0);
    keyFlag.push_back(true);
    treeFlag.push_back(true);
  }

  List out;
  out["X"] = xout;
  out["Y"] = yout;
  out["Z"] = zout;
  out["Intensity"] = votes;
  out["PointSourceID"] = discId;
  out["Keypoint_flag"] = keyFlag;
  out["Radii"] = radii;
  out["TreeID"] = treeId;
  out["TreePosition"] = treeFlag;

  xout.clear();
  xout.shrink_to_fit();
  yout.clear();
  yout.shrink_to_fit();
  zout.clear();
  zout.shrink_to_fit();
  votes.clear();
  votes.shrink_to_fit();
  discId.clear();
  discId.shrink_to_fit();
  keyFlag.clear();
  radii.shrink_to_fit();
  radii.clear();
  treeId.shrink_to_fit();
  treeFlag.clear();
  treeFlag.shrink_to_fit();

  // out["n"] = nPoints;

  return out;
}


// [[Rcpp::export]]
LogicalVector thinCloud(NumericMatrix& las, double voxel = 0.025){
  vector<vector<double> > xyz = rmatrix2cpp(las);
  return wrap(voxelFilter(xyz, voxel));
}

// [[Rcpp::export]]
LogicalVector RCropCloud(NumericMatrix& las, double xCenter, double yCenter, double len, bool circle, bool negative){
  vector<vector<double> > xyz = rmatrix2cpp(las);
  return wrap( cropCloudFilter(xyz, xCenter, yCenter, len, circle, negative) );
}

// [[Rcpp::export]]
List getCircle(NumericMatrix& las, double pixel=0.05, double rad_max=0.25, double min_den=0.1, unsigned int min_votes = 3){

  vector<vector<double> > cloud = rmatrix2cpp(las);
  Raster ras = getCounts(cloud, pixel);
  HoughCenters circle = getSingleCenter(&ras, rad_max, min_den, min_votes);

  List out;
  out["x"] = circle.main_circle.x_center;
  out["y"] = circle.main_circle.y_center;
  out["rad"] = circle.main_circle.radius;
  out["votes"] = circle.main_circle.n_votes;

  return out;
}

// [[Rcpp::export]]
List singleStack(NumericMatrix& las, double pixel=0.05, double rad_max=0.25, double min_den=0.1, unsigned int min_votes = 3){

  vector<HoughCenters> treeMap;
  Raster ras;

  vector<vector<double> > stack = rmatrix2cpp(las);
  ras = getCounts(stack, pixel);
  treeMap = getCenters(&ras, rad_max, min_den, min_votes);
  assignTreeId(treeMap, rad_max, min_den);

  return exportTreeMap(treeMap);

}

// [[Rcpp::export]]
List stackMap(NumericMatrix& las, double hmin=1, double hmax=3, double hstep=0.5, double pixel=0.025, double rad_max=0.25, double min_den=0.1, unsigned int min_votes = 3){

  vector<HoughCenters> treeMap;

  vector<vector<vector<double> > > fullStack = getSlices(las, hmin, hmax, hstep);

  for(auto& stack : fullStack){

    if(stack[0].empty())
      continue;

    Raster ras = getCounts(stack, pixel);
    vector<HoughCenters> tempMap = getCenters(&ras, rad_max, min_den, min_votes);
    treeMap.insert(treeMap.end(), tempMap.begin(), tempMap.end());
  }

  unsigned int nlayers = 0.75 * (hmax - hmin) / hstep;
  assignTreeId(treeMap, rad_max, min_den, nlayers);

  fullStack.clear();
  fullStack.shrink_to_fit();

  return exportTreeMap(treeMap);

}

// [[Rcpp::export]]
List houghStemPoints(NumericMatrix& las, double h1 = 1, double h2 = 3, double hstep=0.5, double radius=0.25, double pixel=0.025, double density=0.1, unsigned int votes=3){

  vector<vector<double> > cppCloud = rmatrix2cpp(las);
  vector<HoughCenters> treeEstimates = treeHough(cppCloud, h1, h2, hstep, radius, pixel, density, votes);

  tempContainer isStem(cppCloud[0].size());
  for(unsigned int i = 0; i < cppCloud[0].size(); ++i){

    double& x = cppCloud[0][i];
    double& y = cppCloud[1][i];
    double& z = cppCloud[2][i];

    if(z < 0) continue;

    unsigned int ptLayer = floor(z / hstep);

    HoughCircle* alias = &treeEstimates[ptLayer].main_circle;

    if(alias->n_votes < votes) continue;

    double dist = sqrt( pow(x - alias->x_center, 2) + pow(y - alias->y_center, 2) );

    if(dist < alias->radius + pixel*2 && dist > alias->radius - pixel*2){
      isStem.filter[i] = true;
      isStem.values[i] = alias->radius;
      isStem.counts[i] = alias->n_votes;
      isStem.sections[i] = ptLayer + 1;
    }
  }

  cppCloud.clear();
  cppCloud.shrink_to_fit();

  List output;
  output["Stem"] = isStem.filter;
  output["Segment"] = isStem.sections;
  output["Radius"] = isStem.values;
  output["Votes"] = isStem.counts;
  isStem.clear();

  return output;
}

// [[Rcpp::export]]
List houghStemPlot(NumericMatrix& las, NumericMatrix& treePositions, double h1 = 1, double h2 = 3, double hstep=0.5, double radius=0.25, double pixel=0.025, double density=0.1, unsigned int votes=3){

  NumericMatrix::Column treecol = treePositions( _, 0);
  NumericMatrix::Column xcol = treePositions( _, 1);
  NumericMatrix::Column ycol = treePositions( _, 2);

  vector<unsigned int> treeIds;
  vector<double> xPos;
  vector<double> yPos;

  treeIds.insert(treeIds.begin(), treecol.begin(), treecol.end());
  xPos.insert(xPos.begin(), xcol.begin(), xcol.end());
  yPos.insert(yPos.begin(), ycol.begin(), ycol.end());

  vector<vector<double> > cloud = rmatrix2cpp(las);
  unordered_map<unsigned int, vector<HoughCenters> > denoisedTrees;

  double cropRadius = radius*4;
  for(unsigned int i = 0; i < treeIds.size(); ++i){
    vector<vector<double> > tree = cropCloud(cloud, xPos[i], yPos[i], cropRadius);
    vector<HoughCenters> denoised = treeHough(tree, h1, h2, hstep, radius, pixel, density, votes);
    denoisedTrees[ treeIds[i] ] = denoised;
  }

  tempContainer plotInfo( cloud[0].size() );
  for(unsigned int i = 0; i < cloud[0].size(); ++i){

    double& x = cloud[0][i];
    double& y = cloud[1][i];
    double& z = cloud[2][i];

    if(z < 0) continue;

    unsigned int ptLayer = floor(z / hstep);
    for(auto& tree : denoisedTrees){

      if(tree.second.size() <= ptLayer) continue;

      HoughCircle* tempCircle = &tree.second[ptLayer].main_circle;

      if(tempCircle->n_votes < votes) continue;

      double dist = sqrt( pow(x - tempCircle->x_center, 2) + pow(y - tempCircle->y_center, 2) );

      if(dist < tempCircle->radius + pixel*2 && dist > tempCircle->radius - pixel*2){
        plotInfo.filter[i] = true;
        plotInfo.values[i] = tempCircle->radius;
        plotInfo.counts[i] = tempCircle->n_votes;
        plotInfo.ids[i] = tree.first;
        plotInfo.sections[i] = ptLayer + 1;
        break;
      }
    }
  }

  cloud.clear();
  cloud.shrink_to_fit();

  List output;
  output["Stem"]   = plotInfo.filter;
  output["TreeID"] = plotInfo.ids;
  output["Segment"] = plotInfo.sections;
  output["Radius"] = plotInfo.values;
  output["Votes"]  = plotInfo.counts;
  plotInfo.clear();

  return output;

}

// [[Rcpp::export]]
NumericVector getCircleRansac(NumericMatrix& las, unsigned int nSamples = 5, double pConfidence = 0.99, double pInliers = 0.8){
  vector<vector<double> > cloud = rmatrix2cpp(las);
  return wrap(ransacCircle(cloud, nSamples, pConfidence, pInliers));
}

// [[Rcpp::export]]
List ransacStem(NumericMatrix& las, std::vector<unsigned int>& segments, std::vector<double>& radii, unsigned int nSamples = 5, double pConfidence = 0.99, double pInliers = 0.8, double tolerance = 0.05){
  vector<vector<double> > cloud = rmatrix2cpp(las);
  return wrap(ransacStemCircles(cloud, segments, radii, nSamples, pConfidence, pInliers, tolerance));
}

// [[Rcpp::export]]
List ransacPlot(NumericMatrix& las, std::vector<unsigned int>& treeId, std::vector<unsigned int>& segments, std::vector<double>& radii, unsigned int nSamples = 5, double pConfidence = 0.99, double pInliers = 0.8, double tolerance = 0.05){
  vector<vector<double> > cloud = rmatrix2cpp(las);
  return wrap(ransacPlotCircles(cloud, treeId, segments, radii, nSamples, pConfidence, pInliers, tolerance));
}
