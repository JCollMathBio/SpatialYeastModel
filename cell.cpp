//cell.cpp

//******************************************
//Include Dependencies
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <math.h>
#include <ctime>
#include <cstdio>
#include <memory>

#include "parameters.h"
#include "coord.h"
#include "cell.h"
#include "colony.h"
//****************************************
//Public Member Functions for Cell.cpp

//construc:tor
Cell::Cell(shared_ptr<Colony> my_colony, int rank, Coord cell_center, double max_radius, double init_radius){
	this->my_colony = my_colony;
	this->rank = rank;
	this->cell_center = cell_center;
	this->max_radius = max_radius;
	this->curr_radius = init_radius;
    this->age = 0;
    this->G1 = true;
    this->G2 = false;
    this->S = false;
    this->M = false;
    this->curr_mother = nullptr;
    this->curr_daughter= nullptr;
    this->is_bud = false;
    this->has_bud = false;

	return;
}
Cell::Cell(shared_ptr<Colony> my_colony, int rank, Coord cell_center, double max_radius, double init_radius, shared_ptr<Cell> mother){
	this->my_colony = my_colony;
	this->rank = rank;
	this->cell_center = cell_center;
	this->max_radius = max_radius;
	this->curr_radius = init_radius;
    this->age = 0;
    this->G1 = true;
    this->G2 = false;
    this->S = false;
    this->M = false;
    this->curr_mother = mother;
    this->curr_daughter= nullptr;
    this->is_bud = true;
    this->has_bud = false;
	return;
}
void Cell::get_daughters(vector<shared_ptr<Cell>>& daughter_cells){
	shared_ptr<Cell> this_cell = shared_from_this();
	daughter_cells = this_cell->daughters;
	return;
}
void Cell::set_daughter(shared_ptr<Cell> daughter){
        this->curr_daughter = daughter;
        return;
}
void Cell::set_mother(shared_ptr<Cell> mother){
        this->curr_mother = mother;
        return;
}

void Cell::reset_is_bud(){
        this->is_bud = false;
        return;
}

void Cell::reset_has_bud(){
        this->has_bud = false;
}

void Cell::grow_cell(){
	if(G1){
		double f_radius = k_g1*curr_radius + k_g2;
		double f_radius_max = k_g1*max_radius + k_g2;
		double slowing_factor = 1- (f_radius/f_radius_max);
		curr_radius = curr_radius + f_radius*slowing_factor*dt;
	}
	return;
}

void Cell::update_cell_cycle(){
	shared_ptr<Cell> this_cell = shared_from_this();
	//cell cycle phase of a bud is started at G1
	//when the bud grows big enough
	//its cell cycle phase gets set to M
	//in addition the cell cycle phase of the mother
	//is set to M so that in the perform_mitosis 
	//function the mother cell and bud are both
	//given the status of regular cell 
	//without bud so the G1 phase is set to 
	//true for both cells
	if(is_bud){
		if(curr_radius > k_mitosis*max_radius){
			this_cell->get_mother()->enter_mitosis();
		}
	}	
	//G1 is set to true when cell object is made
	//if cell gets big enough and there
	//is not already a bud present it enters 
	//S phase so that a bud is created in the
	// perform_budding function. In this function, once 
	//the bud is made the cell cycle phase of the 
	//mother is set to G2 and the cell cycle of the daughter
	//is set to G1.
	else{
		if((curr_radius > k_G1*max_radius) && (!G2)){
			S = true;
		} 
	}

	return;
}

void Cell::enter_mitosis(){
	G1 = false;
	S = false;
	G2 = false;
	M = true;
	return;
}

void Cell::perform_budding(){
	//make bud
    //cout << "cell function" << endl;
    shared_ptr<Cell> this_cell = shared_from_this();
    shared_ptr<Colony> this_colony = this->get_Colony();
    int new_rank = this_colony->get_Num_Cells();
    double init_radius = .2;
    double rand_angle = rand()%360;
    rand_angle = rand_angle/3.14*180;
	double new_center_x = this->cell_center.get_X()+this->curr_radius*cos(rand_angle);
    double new_center_y = this->cell_center.get_Y()+this->curr_radius*sin(rand_angle);
    Coord new_center = Coord(new_center_x, new_center_y);
    auto new_cell = make_shared<Cell>(this_colony, new_rank, new_center, max_radius, init_radius,this_cell);
    this->set_daughter(new_cell);
    this->has_bud = true;
    G1 = false;
    S = false;
    G2 = true;
    M = false;
    this_colony->update_Colony_Cell_Vec(new_cell);
	//new cell is G1 and old cell is G2
	return;
}

void Cell::perform_mitosis(){
	//separate mother and daughter
    this->curr_daughter->reset_is_bud();
    this->reset_has_bud();
    this->curr_daughter->set_mother(nullptr);
    this->set_daughter(nullptr); 
    M = false;
    G1 = true;
    //daughter remains G1 and mother gets set to G1
	return;
}
void Cell::calc_forces_chou(){
    double d_ij; 
    double delta_d;
    Coord v_ij;
    vector<shared_ptr<Cell>> neighbor_cells;
    my_colony->get_Cells(neighbor_cells);
    Coord my_loc = cell_center;
    double my_radius = curr_radius;
    Coord neighbor_loc;
    double neighbor_radius;
    Coord rep_force;
    Coord adh_force;
    shared_ptr<Cell> this_cell = shared_from_this();
    for(unsigned int i=0; i<neighbor_cells.size(); i++){
            if(neighbor_cells.at(i) != this_cell){
            neighbor_loc = neighbor_cells.at(i)->get_Cell_Center();
            neighbor_radius = neighbor_cells.at(i)->get_radius();
            d_ij = (my_loc-neighbor_loc).length();
            delta_d = my_radius + neighbor_radius - d_ij;
            v_ij = (neighbor_loc-my_loc)/d_ij;
            if(d_ij < my_radius+neighbor_radius){
                    rep_force = v_ij*delta_d*k_r*-1;
                    adh_force = v_ij*delta_d*k_a;
            }
    }
    }
    this->curr_force = rep_force +  adh_force;
    return;
}
            
void Cell::calc_forces_jonsson(){
	vector<shared_ptr<Cell>> neighbor_cells;
	my_colony->get_Cells(neighbor_cells);
	Coord my_loc = cell_center;
	double my_radius = curr_radius;
	Coord neighbor_loc;
	double neighbor_radius;
	Coord rep_force = Coord(0,0);
	Coord adh_force = Coord(0,0);
	shared_ptr<Cell> this_cell = shared_from_this();
	Coord diff_vect;
    	double diff_len;
    //cout << rep_force << adh_force << endl;
	for(unsigned int i = 0; i < neighbor_cells.size(); i++){
		neighbor_loc = neighbor_cells.at(i)->get_Cell_Center();
		neighbor_radius = neighbor_cells.at(i)->get_radius();
		diff_vect = neighbor_loc - my_loc;
        	diff_len = diff_vect.length();
        	//cout << (my_loc - neighbor_loc).length() << endl;
		if(neighbor_cells.at(i) != this_cell){
          if((this->is_bud)||(this->has_bud)){
                if((neighbor_cells.at(i) == curr_daughter)){
                    //cout << "Cell rank " << this->rank << " has daughter " << this->curr_daughter->get_rank() << endl;
                    rep_force += (diff_vect/diff_len)*(diff_len-(my_radius/(neighbor_radius+my_radius))*(my_radius+neighbor_radius))*k_spring;
                	    if((my_loc - neighbor_loc).length() < 1.1*(my_radius + neighbor_radius)){
		            	    adh_force += (diff_vect/diff_len)*(diff_len-(my_radius/(neighbor_radius+my_radius))*(my_radius+neighbor_radius))*k_spring*k_adhesion_mother_bud;
                		}		
            	}
            	else if((neighbor_cells.at(i) == curr_mother)){
                    //cout << "Cell rank " << rank << " has mother " << curr_mother->get_rank() << endl;
                	rep_force += (diff_vect/diff_len)*(diff_len-(neighbor_radius/(my_radius+neighbor_radius))*(neighbor_radius+my_radius))*k_spring;
	            		if((my_loc - neighbor_loc).length() < 1.1*(my_radius + neighbor_radius)){
		            		adh_force += (diff_vect/diff_len)*(diff_len -(neighbor_radius/(my_radius+neighbor_radius))*(neighbor_radius+my_radius))*k_spring*k_adhesion_mother_bud;
                		}
            	}
            }
           else {
		//cout << "regular cell" << rank << endl;
                rep_force += (diff_vect/diff_len)*(diff_len - k_repulsion_cell_cell*(my_radius+neighbor_radius))*k_spring;
	                if((my_loc - neighbor_loc).length() < 1.1*(my_radius + neighbor_radius)){
		                adh_force += (diff_vect/diff_len)*(diff_len -k_repulsion_cell_cell*(my_radius+neighbor_radius))*k_spring*k_adhesion_cell_cell;
                
                	}   
	       }
	    }	
    }	
	this->curr_force = rep_force + adh_force;
	//cout << curr_force << endl;
	return;
}

void Cell::calc_forces_exponential(){
    double dd;
    double aaa = .5;
    double ppp = .4;
	vector<shared_ptr<Cell>> neighbor_cells;
	my_colony->get_Cells(neighbor_cells);
	Coord my_loc = cell_center;
	double my_radius = curr_radius;
	Coord neighbor_loc;
	double neighbor_radius;
    double potential_double;
	Coord potential = Coord(0,0);
	Coord adh_force = Coord(0,0);
	shared_ptr<Cell> this_cell = shared_from_this();
	Coord diff_vect;
    double diff_len;
    for(unsigned int i = 0; i < neighbor_cells.size();i++){
            if(neighbor_cells.at(i)!= this_cell){
                neighbor_loc = neighbor_cells.at(i)->get_Cell_Center();
                neighbor_radius = neighbor_cells.at(i)->get_radius();
                diff_vect = (my_loc-neighbor_loc);
                diff_len = diff_vect.length();
                dd = diff_len/((my_radius + neighbor_radius)*.5);
                if(dd < 2){
                    potential_double = exp(-dd*ppp)*dd;
                    potential = potential + (diff_vect/dd)*potential_double/ppp;
                }
            }
    }
    this->curr_force = potential;
    return;
}
                
void Cell::update_location(){
	cell_center = cell_center + curr_force*dt;
	return;
}

double Cell::compute_indica(){
    double d_ij;
    double delta_d;
    vector<shared_ptr<Cell>> neighbor_cells;
    my_colony->get_Cells(neighbor_cells);
    Coord my_loc = cell_center;
    double my_radius = curr_radius;
    Coord neighbor_loc;
    double neighbor_radius;
    double max = 0;
    shared_ptr<Cell> this_cell = shared_from_this();
    for(unsigned int i=0; i<neighbor_cells.size(); i++){
            if(neighbor_cells.at(i) != this_cell){
                neighbor_loc = neighbor_cells.at(i)->get_Cell_Center();
                neighbor_radius = neighbor_cells.at(i)->get_radius();
                d_ij = (my_loc-neighbor_loc).length();
                delta_d = my_radius + neighbor_radius - d_ij;
                if(delta_d > max){
                    max = delta_d;
                }
            }
    }
    return max;
}
    
void Cell::print_txt_file_format(ofstream& ofs){
    ofs << rank << " " << cell_center.get_X() << " " << cell_center.get_Y() << " " << curr_radius << endl;
    return;
}
void Cell::print_cell_center(ofstream& ofs){
    ofs << cell_center.get_X() << " " << cell_center.get_Y() << " " << 0 << endl;
    return;
}
	
