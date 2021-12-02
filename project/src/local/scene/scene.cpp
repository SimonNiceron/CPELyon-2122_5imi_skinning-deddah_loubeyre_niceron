

#include <GL/glew.h>
#include <random>
#include <functional>

#include "scene.hpp"
#include "../../lib/opengl/glutils.hpp"

#include "../../lib/perlin/perlin.hpp"
#include "../../lib/interface/camera_matrices.hpp"

#include "../interface/myWidgetGL.hpp"

#include <cmath>


#include <string>
#include <sstream>
#include "../../lib/mesh/mesh_io.hpp"



using namespace cpe;




static cpe::mesh build_ground(float const L,float const h)
{
    mesh m;
    m.add_vertex(vec3(-L, h,-L));
    m.add_vertex(vec3(-L, h, L));
    m.add_vertex(vec3( L, h, L));
    m.add_vertex(vec3( L, h,-L));

    m.add_triangle_index({0,2,1});
    m.add_triangle_index({0,3,2});

    m.fill_color(vec3(0.8,0.9,0.8));

    return m;
}



//********************************************//
// Build a skinned cylinder along z axis      //
// radius is the radius of the cylinder       // 
// length is the length of the cylinder       //
// Nu is the number of points for each circle //
// Nv is the number of points in the length   //
//********************************************//
static cpe::mesh_skinned build_cylinder(float const radius,float const length,int const Nu,int const Nv)
{
    cpe::mesh_skinned m;

    for (int ku=0; ku<Nu; ku++)
    {
        float u = (2*M_PI/Nu)*ku;        
        for (int kv=0; kv<Nv; kv++)
        {
            float v = (length/(Nv-1))*kv;

            float x = radius*cos(u);
            float y = radius*sin(u);
            float z = v;

            vertex_weight_parameter poids; 
            poids[0].joint_id=0.;
            poids[0].weight=1-float(kv)/float(Nv-1);
            poids[1].joint_id=1.;
            poids[1].weight=float(kv)/float(Nv-1);
            poids[2].joint_id=2;
            poids[2].weight = 0;
            
            m.add_vertex_weight(poids);    //on add les poids relier aux vertex selon si on est sur le premier ou le deuxieme bone
            m.add_vertex(vec3(x,y,z));
        }
    }
    for (int ku=0; ku<Nu; ku++)
    {
        for (int kv=0; kv<Nv-1; kv++)
        {
           int k0 = Nv*ku+kv;
           int k1 = Nv*((ku+1)%Nu)+kv;
           int k2 = Nv*((ku+1)%Nu)+(kv+1);
           int k3 = Nv*ku+(kv+1);

           triangle_index t1(k0, k1, k2);
           triangle_index t2(k0,k2,k3);
           m.add_triangle_index(t1);
           m.add_triangle_index(t2);
        }
    }
    
/* TODO : 
 1) Add Nu*Nv points search as this point belong to the cylinder
 2) Give weights to theses points
 3) Link the points together to form an open cylinder

    Pseudocode :
    for ku in 0:Nu
        u:float <- value in the circle according to ku \in [0,2\pi[ (M_PI is defined)
        for kv in 0:Nv
            v:float <- value in the length according to kv \in [0,length]

            x:float <- x coordinate of the point in the circle in the plane Oxy = radius*cos(u)
            y:float <- y coordinate of the point in the circle in the plane Oxy = radius*sin(u)
            z:float <- z coordinate of the point along the z axis = v

            add vertice (x,y,z) to m
            
            change weights (NOT in the first questions)

    for ku in 0:Nu
        for kv in 0:Nv-1       
            k0:int <- vertex indice of the ku,kv point = Nv*ku+kv
            k1:int <- vertex indice of the next point in the same circle = Nv*((ku+1)%Nu)+kv
            k2:int <- vertex indice of the next point in the next circle = Nv*((ku+1)%Nu)+(kv+1)
            k3:int <- vertex indice of the point in the next circle = Nv*ku+(kv+1)

            add triangle indices for k0, k1, k2
            add triangle indices for k0, k2, k3
*/



    return m;
}

void scene::fill_sk_cylinder_parent_id()
{
    sk_cylinder_parent_id.push_back(-1);
    sk_cylinder_parent_id.push_back(0);
    sk_cylinder_parent_id.push_back(1);
}

void scene::fill_sk_cylinder_bind_pose(int length)
{
    vec3 p0(0,0,0);
    vec3 p1(0,0,length/2);

    quaternion q0(0,0,0,1);
    
    skeleton_joint j0(p0,q0);
    skeleton_joint j1(p1,q0);
    skeleton_joint j2(p1,q0);

    sk_cylinder_bind_pose.push_back(j0);
    sk_cylinder_bind_pose.push_back(j1);
    sk_cylinder_bind_pose.push_back(j2);
}

skeleton_animation scene::remplir_animation(int length)
{
    skeleton_animation ani;

    vec3 p0(0,0,0);
    vec3 p1(0,0,length/2);

    quaternion q0(0,0,0,1);
    
    skeleton_joint j0(p0,q0);
    skeleton_joint j1(p1,q0);
    skeleton_joint j2(p1,q0);

    skeleton_geometry pose;
    pose.push_back(j0);
    pose.push_back(j1);
    pose.push_back(j2);

    ani.push_back(pose);

    float alpha = M_PI/6;
    for (int i=1; i<4; i++)
    {
        q0.set_axis_angle(vec3(1,0,0), alpha*i);
        pose[1].orientation = q0;
        ani.push_back(pose);
        //std::cout<<pose<<std::endl;
    }
    return ani;
}

void scene::load_scene()
{

    //*****************************************//
    // Preload default structure               //
    //*****************************************//
    texture_default = load_texture_file("data/white.jpg");
    shader_mesh     = read_shader("shaders/shader_mesh.vert",
                                  "shaders/shader_mesh.frag");           PRINT_OPENGL_ERROR();
    shader_skeleton = read_shader("shaders/shader_skeleton.vert",
                                  "shaders/shader_skeleton.frag");       PRINT_OPENGL_ERROR();


    //*****************************************//
    // Build ground
    //*****************************************//
    mesh_ground = build_ground(100.0f , -25.0f);
    mesh_ground.fill_empty_field_by_default();
    mesh_ground_opengl.fill_vbo(mesh_ground);

    //*****************************************//
    // Build cylinder
    //*****************************************//

    int lenght = 50;
    mesh_cylinder = build_cylinder(4.0f, lenght, 11, 10);

    fill_sk_cylinder_parent_id();
    fill_sk_cylinder_bind_pose(lenght);

    mesh_cylinder.fill_empty_field_by_default();
    mesh_cylinder_opengl.fill_vbo(mesh_cylinder);

    

    //*****************************************//
    // Build monster
    //*****************************************//
    current_frame_monster = 0;

    texture_monster = load_texture_file("./data/Monster.png");
    mesh_monster.load("./data/Monster.obj");

    sk_monster_bind_pose.load("./data/Monster.skeleton");  
    sk_monster_parent_id.load("./data/Monster.skeleton");

    sk_monster_bind_pose = local_to_global(sk_monster_bind_pose, sk_monster_parent_id); //mettre les joints dans le repere global

    mesh_monster.fill_empty_field_by_default(); 
    mesh_monster_opengl.fill_vbo(mesh_monster);     

    timer1.start();
    timer2.start();
}



void scene::draw_scene()
{
    setup_shader_skeleton(shader_skeleton);
    //*****************************************//
    // Animaton du cylindre
    //*****************************************//

    skeleton_animation sk_ani_cylindre;          //generer une série de skeleton_geometry
    sk_ani_cylindre = remplir_animation(50);

    float curent_time = timer1.elapsed();
    float time_time = 0;
    float time_next = 1000;
    
    //debut boucle de temps
    if (timer1.elapsed() <= 1000)
    {
        current_pos = 0;
        time_time = 0;
        time_next = 1000;
    }    
    else if (timer1.elapsed() <= 2000)
    {
        current_pos = 1;
        time_time = 1000;
        time_next = 2000;
    }
    else if (timer1.elapsed() <= 3000)
    {
        current_pos = 2;
        time_time = 2000;
        time_next = 3000;
    }
    else if (timer1.elapsed() <= 4000)
    {
        current_pos = 3;
        time_time = 3000;
        time_next = 4000;     
    //fin boucle temps
    }
    else {timer1.restart();}

    float alph = (curent_time-time_time)/(time_next-time_time);

    skeleton_geometry const sk_cylinder_global = local_to_global(sk_ani_cylindre(current_pos, alph),sk_cylinder_parent_id); //faire bouger le skeleton
    std::vector<vec3> sk_cylinder_bones;
    sk_cylinder_bones = extract_bones(sk_cylinder_global, sk_cylinder_parent_id);
       

    //Transform the vextex in the vbo according to the new position of the skeleton. 

    setup_shader_mesh(shader_mesh);
    skeleton_geometry const sk_cylinder_inverse_bind_pose = inversed(sk_cylinder_bind_pose);
    skeleton_geometry const sk_cylinder_binded = multiply(sk_cylinder_global,sk_cylinder_inverse_bind_pose);
    mesh_cylinder.apply_skinning(sk_cylinder_binded);
    mesh_cylinder.fill_normal();
    mesh_cylinder_opengl.update_vbo_vertex(mesh_cylinder);
    mesh_cylinder_opengl.update_vbo_normal(mesh_cylinder);

    //*****************************************//
    // Animation du monster
    //*****************************************//

    sk_monster_animation.load("./data/Monster.animations", sk_monster_bind_pose.size());
    if(timer2.elapsed()>200)
    {
        current_frame_monster +=1;
        timer2.restart();
    }
    if(current_frame_monster == sk_monster_bind_pose.size())
    {
        current_frame_monster = 0;
    }

    skeleton_geometry  const sk_monster_global = local_to_global(sk_monster_animation(current_frame_monster, timer2.elapsed()/200.0f), sk_monster_parent_id);
    std::vector<vec3>  sk_monster_bones;
    sk_monster_bones = extract_bones(sk_monster_global, sk_monster_parent_id);
    
  
    skeleton_geometry const sk_monster_inverse_bind_pose = inversed(sk_monster_bind_pose);
    skeleton_geometry const sk_monster_binded = multiply(sk_monster_global, sk_monster_inverse_bind_pose);
    mesh_monster.apply_skinning(sk_monster_binded);

    
    mesh_monster.fill_normal();
    mesh_monster_opengl.update_vbo_vertex(mesh_monster);
    mesh_monster_opengl.update_vbo_normal(mesh_monster); 
    setup_shader_skeleton(shader_skeleton);
    setup_shader_mesh(shader_mesh);
    //*****************************************//
    // dessiner les éléments 
    //*****************************************//
    
    //mesh_cylinder_opengl.draw();
    //draw_skeleton(sk_cylinder_bones); 
    //mesh_ground_opengl.draw();
    mesh_monster_opengl.draw();
    //draw_skeleton(sk_monster_bones);


}



void scene::setup_shader_mesh(GLuint shader_id)
{
    //Setup uniform parameters
    glUseProgram(shader_id);                                                                           PRINT_OPENGL_ERROR();

    //Get cameras parameters (modelview,projection,normal).
    camera_matrices const& cam=pwidget->camera();

    //Set Uniform data to GPU
    glUniformMatrix4fv(get_uni_loc(shader_id,"camera_modelview"),1,false,cam.modelview.pointer());     PRINT_OPENGL_ERROR();
    glUniformMatrix4fv(get_uni_loc(shader_id,"camera_projection"),1,false,cam.projection.pointer());   PRINT_OPENGL_ERROR();
    glUniformMatrix4fv(get_uni_loc(shader_id,"normal_matrix"),1,false,cam.normal.pointer());           PRINT_OPENGL_ERROR();

    //load white texture
    glBindTexture(GL_TEXTURE_2D,texture_default);                                                      PRINT_OPENGL_ERROR();
    glLineWidth(1.0f);                                                                                 PRINT_OPENGL_ERROR();

}

void scene::setup_shader_skeleton(GLuint shader_id, float r , float g, float b )
{
    //Setup uniform parameters
    glUseProgram(shader_id);                                                                           PRINT_OPENGL_ERROR();

    //Get cameras parameters (modelview,projection,normal).
    camera_matrices const& cam=pwidget->camera();

    //Set Uniform data to GPU
    glUniformMatrix4fv(get_uni_loc(shader_id,"camera_modelview"),1,false,cam.modelview.pointer());     PRINT_OPENGL_ERROR();
    glUniformMatrix4fv(get_uni_loc(shader_id,"camera_projection"),1,false,cam.projection.pointer());   PRINT_OPENGL_ERROR();
    glUniform3f(get_uni_loc(shader_id,"color") , r,g,b);                                      PRINT_OPENGL_ERROR();

    //size of the lines
    glLineWidth(3.0f);                                                                                 PRINT_OPENGL_ERROR();
}

void scene::draw_skeleton(std::vector<vec3> const& positions) const
{
    // Create temporary a VBO to store data
    GLuint vbo_skeleton=0;
    glGenBuffers(1,&vbo_skeleton);                                                                     PRINT_OPENGL_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER,vbo_skeleton);                                                        PRINT_OPENGL_ERROR();

    // Update data on the GPU
    glBufferData(GL_ARRAY_BUFFER , sizeof(vec3)*positions.size() , &positions[0] , GL_STATIC_DRAW);    PRINT_OPENGL_ERROR();

    // Draw data
    glEnableClientState(GL_VERTEX_ARRAY);                                                              PRINT_OPENGL_ERROR();
    glVertexPointer(3, GL_FLOAT, 0, 0);                                                                PRINT_OPENGL_ERROR();
    glDrawArrays(GL_LINES,0,positions.size());                                                         PRINT_OPENGL_ERROR();

    // Delete temporary VBO
    glDeleteBuffers(1,&vbo_skeleton);                                                                  PRINT_OPENGL_ERROR();
}

scene::scene()
    :shader_mesh(0)
{}


GLuint scene::load_texture_file(std::string const& filename)
{
    return pwidget->load_texture_file(filename);
}

void scene::set_widget(myWidgetGL* widget_param)
{
    pwidget=widget_param;
}


