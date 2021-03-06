/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	
	// Set the number of particles.
	num_particles = 10;

	// Set a random engine to generate ramdom numbers.
	default_random_engine gen;
	
	// Set random numbers generated by the ramdom engine within a Gaussian distribution.
	normal_distribution<double> dis_x{ x, std[0] };
	normal_distribution<double> dis_y{ y, std[1] };
	normal_distribution<double> dis_theta{ theta, std[2] };

	// Initialize all particles to first position(based on estimates of
	//  x, y, theta and their uncertainties from GPS) and all weights to 1. 
	for (int i = 0; i < num_particles; i++)
		{
			Particle my_particle;
			my_particle.id = i;
			my_particle.x = dis_x(gen);
			my_particle.y = dis_y(gen);
			my_particle.theta = dis_theta(gen);
			my_particle.weight = 1;

			particles.push_back(my_particle);

			weights.push_back(my_particle.weight);
			
		}

	is_initialized = true;
	
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/

	default_random_engine e;
	//cout << "velocity = " << velocity << ", " << "yaw_rate = " << yaw_rate << endl;
	for (int i = 0; i < num_particles; i++)
	{
		double x = particles[i].x;
		double y = particles[i].y;
		double theta = particles[i].theta;
		//cout << "before_pred = " << "(" << x << ", " << y << ")" << ", " << theta << endl;
		if (yaw_rate != 0)
		{
			x = x + (velocity / yaw_rate) * (sin(theta + yaw_rate * delta_t) - sin(theta));
			y = y + (velocity / yaw_rate) * (cos(theta) - cos(theta + yaw_rate * delta_t));
			theta = theta + yaw_rate * delta_t;

		}
		else
		{
			x = x + velocity * cos(theta) * delta_t;
			y = y + velocity * sin(theta) * delta_t;

		}
		//cout << "after_pred = " << "(" << x << ", " << y << ")" << ", " << theta << endl;
		normal_distribution<double> dis_x{ x, std_pos[0] };
		normal_distribution<double> dis_y{ y, std_pos[1] };
		normal_distribution<double> dis_theta{ theta, std_pos[2] };

		particles[i].x = dis_x(e);
		particles[i].y = dis_y(e);
		particles[i].theta = dis_theta(e);
		particles[i].weight = 1; // Initialize the weight of each particle back to 1.

	}
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html
	
	double std_x = std_landmark[0];
	double std_y = std_landmark[1];
	
	for (int i = 0; i < num_particles; i++)
	{
		double weight = particles[i].weight;
		double x_p = particles[i].x;
		double y_p = particles[i].y;
		double theta_p = particles[i].theta;
		//cout << "particle = (" << particles[i].x << ", " << particles[i].y << ")" << endl;// This line for debugging.
		
		// Pick out those landmarks within sensor range for each of the particles.
		vector<LandmarkObs> surroundinglandmarks;

		for (int i = 0; i < map_landmarks.landmark_list.size(); i++)
		{
			double dis = dist(x_p, y_p, map_landmarks.landmark_list[i].x_f, map_landmarks.landmark_list[i].y_f);

			if (dis <= sensor_range)
			{
				LandmarkObs surroundinglandmark;

				surroundinglandmark.id = map_landmarks.landmark_list[i].id_i;
				surroundinglandmark.x = map_landmarks.landmark_list[i].x_f;
				surroundinglandmark.y = map_landmarks.landmark_list[i].y_f;

				surroundinglandmarks.push_back(surroundinglandmark);

			}
		}
		
		// Transform the observations from car coordinates to map coordinates.
		vector<LandmarkObs> trans_obs;
		
		for (int i = 0; i < observations.size(); i++)
		{
			double x_c = observations[i].x;
			double y_c = observations[i].y;
			
			double x_m = x_p + x_c * cos(theta_p) - y_c * sin(theta_p);
			double y_m = y_p + x_c * sin(theta_p) + y_c * cos(theta_p);
			
			LandmarkObs trans_ob;
			
			trans_ob.id = observations[i].id;
			trans_ob.x = x_m;
			trans_ob.y = y_m;
			
			trans_obs.push_back(trans_ob);
			
		}
		
		// Associate each of the transformed observations with the nearest landmark && Update the weights.
		for (int i = 0; i < trans_obs.size(); i++)
		{
			double x = trans_obs[i].x;
			double y = trans_obs[i].y;
			double x_diff = x - surroundinglandmarks[0].x; // Difference of x between transformed observations and the first map landmarks.
			double y_diff = y - surroundinglandmarks[0].y; // Difference of y between transformed observations and the first map landmarks.
			
			double min_dis = dist(x, y, surroundinglandmarks[0].x, surroundinglandmarks[0].y);
			
			int id = surroundinglandmarks[0].id;
			double x_lm = surroundinglandmarks[0].x; 
			double y_lm = surroundinglandmarks[0].y;
			
			// Calculate the difference between transformed observations and other landmarks and find the associated(nearest) landmark.
			for (int i = 1; i < surroundinglandmarks.size(); i++)
			{
				double dis = dist(x, y, surroundinglandmarks[i].x, surroundinglandmarks[i].y);
				
				if (dis < min_dis)
				{
					min_dis = dis;
					x_diff = x - surroundinglandmarks[i].x;
					y_diff = y - surroundinglandmarks[i].y;

					id = surroundinglandmarks[i].id;
					x_lm = surroundinglandmarks[i].x;
					y_lm = surroundinglandmarks[i].y;
				}
			}
			// This line for debugging.
			//cout << "obs_original = (" << observations[i].x << ", " << observations[i].y << "), obs_trans = (" << x << ", " << y << "), ass_lm_id = " << id << ", " << " ass_lm = (" << x_lm << ", " << y_lm << ")" << endl;
			
			double exponent = 0.5 * (pow(x_diff / std_x, 2) + pow(y_diff / std_y, 2));
			weight = weight * exp(-exponent) / (2 * M_PI * std_x * std_y);
			
		}		
		particles[i].weight = weight; // Update the weight of each particle.
		//cout <<" particle_weight = " << particles[i].weight << endl;// This line for debugging.
		weights[i] = weight; // Update the vector of weights.		
	}
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution

	default_random_engine gen;

	discrete_distribution<int> dis{weights.begin(), weights.end()};

	vector<Particle> new_particles;

	for (int i = 0; i < num_particles; i++)
	{
		int index = dis(gen);
		
		new_particles.push_back(particles[index]);		
	}

	particles = new_particles;
}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
