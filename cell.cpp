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

void Cell::set_is_bud(bool bud_status){
        this->is_bud = bud_status;
        return;
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
    double init_radius = .01;
	Coord new_center = this->cell_center;
    auto new_cell = make_shared<Cell>(this_colony, new_rank, new_center, max_radius, init_radius,this_cell);
    this->set_daughter(new_cell);
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
    this->curr_daughter->set_mother(nullptr);
    this->curr_daughter->set_is_bud(false);
    this->curr_daughter = nullptr;
    M = false;
    G1 = true;
    //daughter remains G1 and mother gets set to G1
	return;
}
void Cell::calc_forces(){
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
            if((neighbor_cells.at(i) == curr_daughter)){
                rep_force += (diff_vect/diff_len)*(diff_len-.9*(my_radius))*k_spring;
                if((my_loc - neighbor_loc).length() < 1.1*(my_radius + neighbor_radius)){
		            adh_force += (diff_vect/diff_len)*(diff_len -k_repulsion_cell_cell*(neighbor_radius))*k_spring*k_adhesion_mother_bud*-1;
                }
            }
            else if((neighbor_cells.at(i) == curr_mother)){
                rep_force += (diff_vect/diff_len)*(diff_len - (.9)*(my_radius+neighbor_radius))*k_spring;
	            if((my_loc - neighbor_loc).length() < 1.1*(my_radius + neighbor_radius)){
		            adh_force += (diff_vect/diff_len)*(diff_len -k_repulsion_cell_cell*(my_radius+neighbor_radius))*k_spring*k_adhesion_mother_bud*-1;
                }
            }
            else {
                rep_force += (diff_vect/diff_len)*(diff_len - k_repulsion_cell_cell*(my_radius+neighbor_radius))*k_spring;
	            if((my_loc - neighbor_loc).length() < 1.1*(my_radius + neighbor_radius)){
		            adh_force += (diff_vect/diff_len)*(diff_len -k_repulsion_cell_cell*(my_radius+neighbor_radius))*k_spring*k_adhesion_cell_cell*-1;
                
                }   
	        }
	    }
    }
	this->curr_force = rep_force + adh_force;
	//cout << curr_force << endl;
	return;
}
void Cell::update_location(){
	cell_center = cell_center + curr_force*dt;
	return;
}
void Cell::print_txt_file_format(ofstream& ofs){
    ofs << rank << " " << cell_center.get_X() << " " << cell_center.get_Y() << " " << curr_radius << endl;
    return;
}
void Cell::print_cell_center(ofstream& ofs){
    ofs << cell_center.get_X() << " " << cell_center.get_Y() << " " << 0 << endl;
    return;
}
	
