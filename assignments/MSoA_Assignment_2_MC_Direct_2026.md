**Assignment #2**

![](<MSoA_Assignment_2_MC_Direct_2026_files/img0001.png>)

# Monte Carlo Direct Illumination

**Modeling and Simulation of Appearance**

*Master on Robotics, Graphics and Computer Vision*

# Overview

In this assignment you will implement your first Monte Carlo renderer working with direct illumination. It will generalize the point-light renderer you implemented on Assignment #1, so that it efficiently handles multiple area light sources of arbitrary shape. After completing this assignment, you will be able to render area light sources and soft shadows with your rendering code. Note that this assignment builds on code from Assignment #1.

This assignment consists of two parts. In the first, you will get to implement Monte Carlo sampling techniques that warp an initially uniform distribution into a number of different target distributions. Note that all work you do in this assignment will serve as building blocks in later assignments when we apply Monte Carlo integration to render images.

In the second part, you will implement a very basic stochastic rendering algorithm, which computes direct light coming from area light sources and environment maps.

**Important:**

Note that this assignment assumes you implemented

src/pointlight.cpp and src/direct\_whitted.cpp in Assignment #1.

**Important:** You do not need to implement every part of this assignment to earn the maximum grade. There are two parts marked as **Extra** (one in Section 1, the other in Section 2). The parts that do not have an **Extra** label are mandatory, and their code will be re-used in future assignments.

# 0 Preliminaries

First, **download** the new scenes from the patch in Moodle (extract assignment-2/\* to scenes/assignment-2/\*). Then, before coding, compile and run

`$ ./nori ../scenes/assignment-2/aliasing/aliasing.xml`

Running this will render a scene similar to Figure 1 (left), with significant artifacts due to poorly sampling a high-frequency signal (the checkerboard). This is because a single (Monte Carlo) sample per pixel has been used here. Now open the scene at ../scenes/assignment- 2/aliasing.xml, and at the beginning of the scene description you will see the following lines

1

![](<MSoA_Assignment_2_MC_Direct_2026_files/img0002.jpg>)

![](<MSoA_Assignment_2_MC_Direct_2026_files/img0003.jpg>)

Figure 1: ***Left:*** *Example of aliasing due to undersampling a high-frequency signal (the checker-*

*board), rendered with a single sample per pixel.* ***Right:*** *A noise-free render of the same scene with* *×64* *more samples per pixel; note how the reconstructed image is now smooth.*

```

\<!-- Independent sample generator --\>\
\<sampler type\="independent"\>

\<integer name\="sampleCount" value\="1"/\>\
\</sampler\>

```

there, you can set the number of samples (per pixel) used to render your image. Setting sampleCount to 64, and rendering again, you will obtain an artifact-free version of the previous image (Figure 1, right), at the price of a significantly larger computation time (×64). In the following, set your sample count by changing this parameter in all the xml files describing the scene.

**Tip:** You can also override the number of samples per pixel from the command line using the --spp (or -s) option, e.g. ./nori ../scenes/assignment- 2/aliasing/aliasing.xml -s 32 will render with 32 spp.

# 1 Monte Carlo Sampling (40%)

In this exercise you will generate sample points on various domains: the plane, spheres, and hemispheres. In particular, you are asked to implement a distribution function (a *pdf* ) and a matching sample warping scheme. It is crucial that both are consistent with respect to each other (i.e. that warped samples have exactly the distribution described by the density function). Significant errors can arise if inconsistent warpings are used for Monte Carlo integration.

2

To validate the consistency of the warping procedures and associated pdfs, the base code includes an interactive visualization and testing tool to make working with point sets as intuitive as possible. After compiling all the code, you should see an executable named warptest. Run this executable to launch the interactive warping tool, which allows you to visualize the behavior of different warping functions given a range of input point sets (independent, grid and stratified). Up to now, we only discussed uniform random variables which correspond to the ”independent” type, and you need not concern yourself with the others for now. The provided warptest tool implements a *χ* test to ensure that this consistency requirement is indeed satisfied. Note that passing the test does not generally imply that your implementation is correct–for instance, the test may not have enough ”evidence” to generate a failure, or potentially the warping function and the density function are both incorrect in the same manner. Use your judgment and do not rely on this test alone.

2

**What you have to do** Implement the following missing functions in class Warp found in src/warp.cpp. This class consists of various warp methods that take as input a 2D point (s,t) *∈* \[0,1) *×* \[0,1) and return the warped 2D (or 3D) point in the new domain. Each method is accompanied by another method that returns the probability density with which a sample was picked. Our default implementations all throw an exception, which shows up as an error message in the graphical user interface of warptest. The slides on the course website provide a number of useful recipes for warping the samples and computing the densities, and the PBR textbook also contains considerable information on this topic that you should feel free to use.

**Hint:** When developing your sampling routines, remember that:

• A uniform distribution on a surface has probability *p(x)* \= 1/A with *A* the area of the surface.

• In the case of constant *p(x),* take into account that when *f(x)* \= 0 its pdf *p(x)* \= 0 (e.g. when uniformly sampling a 2D shape, *p(x)* \= 0 when **x** outside of the shape).

• **Warp::squareToUniformTriangle** **and** **Warp:squareToUniformTrianglePdf** **(10%):** Implement a method that transforms uniformly distributed 2D points on the unit square into uniformly distributed points on a triangle. Fill in code for uniformly sampling points from a unit triangle. Next, implement a probability density function that matches your warping scheme.

• **Warp::squareToUniformSphere** **and** **Warp::squareToUniformSpherePdf** **(10%):** Implement a method that transforms uniformly distributed 2D points on the unit square into uniformly distributed points on the unit sphere centered at the origin. Implement

a matching probability density function.

3

• **Warp::squareToUniformHemisphere** **and** **Warp::squareToUniformHemispherePdf** **(5%):** Implement a method that transforms uniformly distributed 2D points on the

unit square into uniformly distributed points on the unit hemisphere centered at the origin and oriented in direction (0,0,1). Add a matching probability density function.

• **Warp::squareToCosineHemisphere** **and** **Warp::squareToCosineHemispherePdf** **(5%):** Transform your 2D point to a point distributed on the unit hemisphere with a cosine density function *p(ω)* \=

cosθω

, where *θω* is the angle between a direction *ω* on

*π*

the hemisphere and the north pole (i.e. direction (0,0,1) ).

• **Warp::squareToBeckmann** **and** **Warp::squareToBeckmannPdf** **(10%):** Transform your 2D point to a point distributed on the unit hemisphere with a Beckmann half-vector distribution:

*−* tan 2 *θ*

*h*

*α* 2

*e*\
*π α* cos *θh*

*D(ωh)* \=

2 4 *,*    (1)

where *α* is a user-specified roughness parameter, and *θh* is the half angle defining the angle between the half vector *ωh* and the north pole. Remember that the pdf *p(ωh)* of a normal distribution function *D(ωh)* is *p(ωh)* *∝* *D(ωh)cosθh,* and therefore it holds

*−* tan 2 *θ*

*π*    *π*

∫ 2π ∫    ∫ 2π ∫

*α* 2

2    2

*e*

*p(ωh)* \= *C*

*D(ω)* cosθ sinθ dθ dφ \= *C*

2 3 sinθ dθ dφ \= 1

0 0    0 0 *π α* cos *θ*

(2)

for the normalization constant *C.* Use this identity to normalize your pdf.

**Extra** **Warp::squareToUniformDisk** **and** **Warp::squareToUniformDiskPdf** **(10%):**

Implement a method that transforms uniformly distributed 2D points on the unit square into uniformly distributed points on a planar disk with radius 1 centered at the origin. Next, implement a probability density function that matches your warping scheme.

Remember that you should validate whether your sampling routines are correct, passing all *χ* tests of the above warpings using warptest.exe. Take screenshots of the result of the test for each implemented warping method and store them as png images with names UniformTriangle.png, UniformSphere.png, UniformHemisphere.png, CosineHemisphere.png, Beckmann.png, and optionally UniformDisk.png, respectively; submit these images following the instructions at the end of this document.

2

# 2 Direct Light – Emitter Sampling (60%)

In this second part, you will extend the simple Whitted-style renderer you implemented in Assignment #1 (direct\_whitted.cpp) to take advantage of Monte Carlo integration when handling arbitrary light sources beyond simple point lights.

4

Your new integrator will compute the local illumination integral discussed in class:

*Lo(x,ωo)* \= *Le(x,ωo)* + *Lr* (x,ωo)    (3)

∫

\= *Le(x,ωo)* + *Li(x,ωi)fr* (x,ωi,ωo) cosθi dωi,    (4)

Ω

where *Lo* is the outgoing radiance at a surface position **x** in direction *ωo* as the sum of *Le* and *Lr* the emitted and reflected radiance at **x** respectively. The reflected radiance is defined as the integral on the hemisphere Ω of the incoming radiance *Li* at **x** from direction *ωi* *∈* Ω, times the BRDF *fr* . The angle *θi* is the angle between *ωi* and the surface normal at **x.** Generally, *Li* and *Lo* are related to each other using the ray tracing operator r(x,ω), which returns the nearest surface position visible along the ray with origin **x** and direction *ω,* i.e.

*Li(x,ω)* \= *Lo(r(x,ω),−ω),*    (5)

and the above integral is thus defined recursively. In this assignment, we focus on direct illumination only and therefore truncate the recursion after the first light bounce. This means that the integral simplifies to

∫

*Lo(x,ωo)* \= *Le(x,ωo)* + *Le(r(x,ωi),−ωi)fr* (x,ωi,ωo) cosθi dωi.

(6)

Ω

During rendering, Nori will approximate this integral with the Monte Carlo estimate

∑ *N* (    )

1    (k)    (k)    (k)    (k)

*Lo(x,ωo)* *≈*

*Le(x,ωo)* + *Le(r(x,ω* *i* ),−ω *i* )fr (x,ωo,ω *i* ) cosθ *i* *,* (7)

*N*

*k\=1*

(k)

where *ω* *i* is the **sampled** incoming direction. Note that the summation and averaging is taken care of by Nori itself, and you do not need to write code for this. The integrator in this assignment (and future ones) only need to compute the values of a single sample at a time (i.e. the inner term of the sum in Equation (7)).

We will be working with emitters that have a finite extent, which means that they can be directly observed by the camera. This is why the first summand (Le) in Equation (7) is needed: It returns emitted radiance towards the camera when light sources are visible on screen. Also note that Equation (7) defines outgoing radiance at surface points. To visualize incident radiance at the camera, you will need one more invocation of the ray tracing operator. Like in the first assignment, we provide an API that you should use to implement the sampling and evaluation operations for emitters, as well as storage and query of emitters in the scene.

To complete this task, you will need to fill in code for the following steps:

1\. Generate a sample from a randomly chosen light source, and evaluate its direct contribution (direct\_ems.cpp). Remember that Nori will call the integrator several times per pixel, and average each individual contribution to evaluate the sum in Equation (7).

5

2\. Uniformly sample a point on a triangle mesh (mesh.h and mesh.cpp).

3\. Generate samples from a triangle mesh, and computer the sample radiance/PDF (area.cpp).

To help you with completing the assignment, we’ve broken up the steps into smaller subtasks:

## 2.1 Integrator (20%)

Create a direct\_ems.cpp file, and create a new integrator called DirectEmitterSampling. Use the NORI\_REGISTER\_CLASS to register the new integrator as ”direct\_ems” as:

NORI\_REGISTER\_CLASS(DirectEmitterSampling, "direct\_ems");

Use DirectWhittedIntegrator that you implemented in direct\_whitted.cpp as a base for this new integrator. Note that DirectWhittedIntegrator does iterate over all light sources (see l.31 of direct\_whitted.cpp) and does not account for visible light sources.

Do not forget to add direct\_ems.cpp to CMakeLists.txt and run CMake before compiling.

The DirectEmitterSampling integrator should approximate the Equation (6) as follows

∑ *N*

(k)    (k)    (k)    (k)

*Le(r(x,ω* *i* ),−ω *i* )fr (x,ωo,ω *i* ) cosθ *i*

1

*Lo(x,ωo)* *≈* *Le(x,ωo)* +    (k)    (8)

*N*    *pΩ(ω* )

*k\=1*    *i*

∑ *N*

(k) (k)

(k)    (k)    (k)

*Le(x* *l* *,−ω* *i* )V (x *l* *→* **x)fr** (x,ωo,ω *i* ) cosθ *i*

1

\= *Le(x,ωo)* +    (k)    (9)

*N*    *pΩ(x,x* )

*k\=1*    *l*

(k)    (k)

where *ω* *i* \= *\|x* *l* *−* *l\|* was generated using emitter sampling (i.e. sampling the position **x** *l* at the light source). For that, first randomly sample a light source in the scene using Scene::sampleEmitter; then, sample the light source to obtain the sampled EmitterQueryRecord using Emitter::sample. Do not forget to initialize your sample point at EmitterQueryRecord::ref to the point you are illuminating **before** sampling. Remember that *pΩ(x,x* *l* ) is the product of the pdf of choosing the light source *l* and the pdf of **x** *l* at the light source.

(k)

(k)

(k)

Finally, make sure to include the directly visible radiance (the first *Le* term). You can check if the intersected material is an emitter using its.mesh-\>isEmitter, and may call its Emitter::eval method to compute the visible radiance.

6

![](<MSoA_Assignment_2_MC_Direct_2026_files/img0004.jpg>)

![](<MSoA_Assignment_2_MC_Direct_2026_files/img0005.jpg>)

Figure 2: *The* Serapis *bust illuminated by seven colored point light sources, rendered using the*

*DirectWhittedIntegrator* *(left) and the* *DirectEmitterSampling* *integrator (right) with one sample per pixel. The rendering cost of each image is 453 and 137 ms, respectively. Note that in both cases there is significant aliasing in the borders, though in the image on the right it is masked by the Monte Carlo noise of light sampling.*

Demonstrate your integrator by rendering the two scenes serapis\_whitted.xml and serapis\_ems.xml in folder scenes/assignment-2/serapis/: The first one will render the scene by using the DirectWhittedIntegrator from Assignment #1, while the second will use DirectEmitterSampling, in both cases with one sample per pixel. The result should look like something like Figure 2, where DirectWhittedIntegrator shows a significant aliasing in the borders but the shading is smooth, while DirectEmitterSampling exhibits lots of variance since only one light source is randomly sampled at each sample. Now increase the sample budget of both scenes to 64 samples per pixel. In both cases the result should converge to the exact same solution.

**Extra 10%:** To improve convergence, you can implement importance sampling of the light sources, so that brighter sources would potentially receive more samples. In order to support importance sampling, update Scene::sampleEmitter so that the probability of sampling a light source is proportional to their emitted radiance. You can use DiscretePDF for sampling the light sources. You can construct your discrete sampler before you start rendering, in Scene::activate. Show the improvement of using the naïve sampling against the radiance-based one.

## 2.2 Mesh Area Light (30%)

Here you will implement a new type of emitter: an *area light.* As opposed to the point lights implemented in Assignment #1, area lights have finite area. In this case, you will implement area lights associated to a triangle Mesh. For convenience, we split this task into multiple subtasks.

7

**Triangle Sampling** Familiarize yourself with the Mesh class (Mesh.cpp and Mesh.h) to see how vertices, faces and normals are stored. Fill the function Mesh::samplePosition that samples a point in a triangle of the mesh. Use Warp::squareToUniformTriangle from Part 1 to uniformly sample the barycentric coordinates of the triangle, and use these barycentric coordinates to interpolate between the positions of the triangle’s vertices. You should compute the sampled point, the surface normal, and the UV coordinates (if any) at that point. For now, assume the mesh only has a single triangle.

Then extend your code for Mesh::samplePosition to handle multiple triangles. To do this, sample a triangle of the mesh at random, with probability proportional to the triangle area: You can use DiscretePDF Mesh::m\_pdf (see include/dpdf.h for details on DiscretePDF) for sampling all triangles. Fill the function Mesh::pdf that returns the PDF of sampling a point on the mesh (Hint: For uniform distributions, the PDF is always 1/area; you can compute the area of each triangle by using the function Mesh::surfaceArea, and the area of the full mesh is the normalization factor of the discrete PDF, which you can obtain through DiscretePDF::getNormalization). Note that DiscretePDF needs to be initialized before use; do that in Mesh::activate.

**Hint:** The function Mesh::samplePosition receives a 2D random sample point. You can reuse one of the dimensions for sampling both the triangle and one barycentric coordinate of the triangle. For that you can use the function DiscretePDF::sampleReuse() for both sampling the triangle, and updating the random sample.

**Area Emitter** Take a look at area.cpp. The AreaEmitter is a helper class that can be attached to a Mesh to turn it into an emitter. Fill in AreaEmitter::eval: This method should return the radiance of the light, as seen from a specified direction. This function takes in an EmitterQueryRecord, which is defined in emitter.h. Be careful to check that the outgoing direction is above the emitting hemisphere of the triangle. Also note that AreaEmitter supports textured area lights; you can access the radiance of these area lights using AreaEmitter::m\_radiance-\>eval(), passing the texture coordinates in the introduced EmitterQueryRecord.

Using the methods implemented in mesh.cpp, fill in AreaEmitter::sample and its corresponding pdf in AreaEmitter::pdf. Remember to convert the positional pdf *pS* returned by Mesh::pdf to solid angle *pΩ* measure. These are related by the factor

2

*\|\|x −* *xl\|\|*\
*\|nl ·* *ωi\|*

*pΩ(x,xl)* \= *pS* (xl)

*.*    (10)

8

![](<MSoA_Assignment_2_MC_Direct_2026_files/img0006.png>)

![](<MSoA_Assignment_2_MC_Direct_2026_files/img0007.png>)

Figure 3: *Left: Converged render of the Serapis bust with environment lighting. Right: you can try*

*downloading higher quality envmaps from HDRIHaven and use them in the scene. This example can be found in* [*https://polyhaven.com/a/citrus\_orchard\_road\_ puresky* ](<https://polyhaven.com/a/citrus_orchard_road_puresky>)*(CC0 license).*

**Hint:** To help you narrow down bugs, you can test your integrator for a scene with a single emitter and a single triangle. This scene setup excludes any potential bugs introduced from incorrect use of DiscretePDF.

Also, be careful when filling the sampled normal in Mesh::samplePosition, since some meshes do not have explicit normals in their vertices (i.e. the normals vector Mesh::m\_N might be empty): In this case, you must compute explicitly the normal of the triangle. An example on how to compute the point, normal, and UV coordinates of a point from the barycentric coordinates of a triangle in Nori can be found in l.452-493 of accel.cpp.

We have prepared a couple of scenes for demonstrating your code: bunny, cbox, and table. Of course, you are more than welcome to prepare additional scenes to demonstrate your code. For helping with comparisons, we have also included a converged version of the cbox scene (1024 spp), which you can use as ground truth (see scenes/assignment- 2/references).

## 2.3 Environment Light (10%)

Here you will implement a new type of emitter: an *environment light.* This type of light source is a spherical light source that is placed at the infinite. Think of it as the light coming from the sky. While the basic stuff is already implemented in environment.cpp, you need to fill the sampling routines needed to support illumination from the environment map. In particular, implement the functions EnvironmentEmitter::sample() and EnvironmentEmitter::pdf() by uniformly sampling a direction on the sphere. You can use the provided serapis/serapis\_env.xml to demonstrate your code. We also include a converged render (2048 spp) that you can use as ground truth for validation (see scenes/assignment-2/references), or Figure 3.

9

# 3 References

You may find the following general references useful:

• *”Physically Based Rendering, Third Edition: From Theory To Implementation”* by Matt Pharr, Wenzel Jakob, and Greg Humphreys. Morgan Kaufmann, 3rd edition, Nov 2016. [http://www.pbr-book.org/](<http://www.pbr-book.org/>)

• *”Global Illumination Compendium - The Concise Guide to Global Illumination Algorithms”,* Philip Dutre, 2003. [https://people.cs.kuleuven.be/\~philip. dutre/GI/](<https://people.cs.kuleuven.be/~philip.dutre/GI/>)

Feel free to consult additional references when completing projects, but remember to properly acknowledge them in your submission. When asked to implement feature X, we request that you don’t go and read the source code of the implementation of X in some other renderer, because you will likely not learn much in the process. The PBR book is excluded from this rule. If in doubt, get in touch with the course staff.

# 4 What to submit?

You should submit all your rendered figures and the source code you generated in this assignment. The submission should be a a zip file with name p2\_NIP1\_NIP2.zip, and with the following structure:

• A file README.txt including the name of the authors (as sanity check) and all references consulted during the development of the assignment. This file should also contain a disclaimer about your usage of generative AI for the code, in which you need to explain in around two paragraphs which coding tasks have been done by generative AI, why have you needed generative AI for each coding task and which AI model have you used.

• A folder ./figures including all generated figures (both exr and png); the figures should be named with the name of the xml scene, appended with the number of samples used to generate these figures (e.g. if you generated a render of the scene table\_env.xml with 16 samples per pixel, then you should submit the figures table\_env\_16.exr and table\_env\_16.png). Do not forget to include screenshots from Part 1 of the assignment.

• A folder ./src including warp.cpp, area.cpp, mesh.cpp, direct\_ems.cpp, environment.cpp, and any other source file you modified or added in your implementation.

In case your file gets too big for the submission system, you can just include a link to a shared folder (Google Drive, OneDrive, etc.) with additional figures in README.txt. You

10

will find different submission entries and corresponding deadlines for each group in Moodle. Please submit your assignment in the entry corresponding to your group.

11
