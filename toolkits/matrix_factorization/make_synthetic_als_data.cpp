/**  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */

#include <Eigen/Dense>
#include <graphlab.hpp>

typedef Eigen::VectorXd vec_type;
typedef Eigen::MatrixXd mat_type;


#include <graphlab/util/stl_util.hpp>
#include <graphlab/macros_def.hpp>


int main(int argc, char** argv) {
  global_logger().set_log_level(LOG_INFO);
  global_logger().set_log_to_console(true);

  // Parse command line options -----------------------------------------------
  const std::string description = 
    "Creates a folder with synthetic training data";
  graphlab::command_line_options clopts(description, false);
  std::string output_folder = "synthetic_data";
  size_t nfiles            = 5;
  size_t D                 = 20;
  size_t nusers            = 1000;
  size_t nmovies           = 10000;
  size_t nvalidation       = 2;
  double noise             = 0.1;
  double stdev             = 2;
  double alpha             = 1.8;


  clopts.attach_option("dir", &output_folder, output_folder,
                       "Location to create the data files");
  clopts.attach_option("nfiles", &nfiles, nfiles,
                       "The number of files to generate.");
  clopts.attach_option("D", &D, D, "Number of latent dimensions.");
  clopts.attach_option("nusers", &nusers, nusers,
                       "The number of users.");
  clopts.attach_option("nmovies", &nmovies, nmovies,
                       "The number of movies.");
  clopts.attach_option("alpha", &alpha, alpha,
                       "The power-law constant.");
  clopts.attach_option("nvalidation", &nvalidation, nvalidation,
                       "The validation ratings pers user");
  clopts.attach_option("noise", &noise, noise,
                       "The standard deviation noise parameter");
  clopts.attach_option("stdev", &stdev, stdev,
                       "The standard deviation in latent factor values");

  if(!clopts.parse(argc, argv)) {
    std::cout << "Error in parsing command line arguments." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Creating data directory: " << output_folder << std::endl;
  boost::filesystem::path directory(output_folder);
  if(!boost::filesystem::create_directory(output_folder)) {
    logstream(LOG_ERROR) 
      << "Error creating directory: " << directory << std::endl;
  }

  std::cout << "Opening files:" << std::endl;
  std::vector< std::ofstream* > train_files(nfiles);
  std::vector< std::ofstream* > validation_files(nfiles);
  for(size_t i = 0; i < nfiles; ++i) {
    const std::string train_fname = 
      output_folder + "/graph_" + graphlab::tostr(i) + ".tsv";
    train_files[i] = new std::ofstream(train_fname.c_str());
    if(!train_files[i]->good()) {
      logstream(LOG_ERROR) 
        << "Error creating file: " << train_fname;
    }

    const std::string validation_fname = 
      output_folder + "/graph_" + graphlab::tostr(i) + ".tsv.validate";
    validation_files[i] = new std::ofstream(validation_fname.c_str());
    if(!validation_files[i]->good()){
      logstream(LOG_ERROR) 
        << "Error creating file: " << train_fname;
    }       
  }
  



  // Make synthetic latent factors
  std::vector< vec_type > user_factors(nusers);
  std::vector< vec_type > movie_factors(nmovies);
  // Create a shared random number generator
  graphlab::random::generator gen; gen.seed(31413);
  
  std::cout << "Constructing latent user factors" << std::endl;
  foreach(vec_type& factor, user_factors) {
    factor.resize(D);
    // Randomize the factor
    for(size_t d = 0; d < D; ++d) 
      factor(d) = gen.gaussian(0, stdev);
  }

  std::cout << "Constructing latent movie factors" << std::endl;
  foreach(vec_type& factor, movie_factors) {
    factor.resize(D);
    // Randomize the factor
    for(size_t d = 0; d < D; ++d) 
      factor(d) = gen.gaussian(0, stdev);
  }

  ASSERT_GT(nusers, nvalidation);  
  // Make power-law probability vector
  std::vector<double> prob(nusers - nvalidation);
  for(size_t i = 0; i < prob.size(); ++i)
    prob[i] = std::pow(double(i+1), -alpha);
  graphlab::random::pdf2cdf(prob);
  for(size_t movie_id = 0, user_id = 0; movie_id < nmovies; ++movie_id) {
    // Add power-law out degree ratings
    const size_t out_degree = gen.multinomial_cdf(prob) + 1;
    for(size_t i = 0; i < out_degree; ++i) {
      user_id = (user_id + 2654435761)  % nusers;
      const size_t file_id = user_id % nfiles;
      const double rating = 
        user_factors[user_id].dot(movie_factors[movie_id]);
      *(train_files[file_id])
        << user_id << '\t' << (movie_id + nusers) << '\t' << rating << '\n';
    }
    // Add a few extra validation ratings
    for(size_t i = 0; i < nvalidation; ++i) {
      user_id = (user_id + 2654435761)  % nusers;
      const size_t file_id = user_id % nfiles;
      const double rating = 
        user_factors[user_id].dot(movie_factors[movie_id]);
      *(validation_files[file_id])
        << user_id << '\t' << (movie_id + nusers) << '\t' << rating << '\n';
    }
  } // end of loop over movies
  for(size_t i = 0; i < nfiles; ++i) {
    train_files[i]->close(); 
    delete train_files[i]; train_files[i] = NULL;
    validation_files[i]->close(); 
    delete validation_files[i]; validation_files[i] = NULL;
  }



} // end of main
