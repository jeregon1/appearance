**Assignment #4**

![](<MSA_Assignment_4_Path_Tracing_2026_files/img0001.png>)

# Path Tracing

**Modeling and Simulation of Appearance**

*Master on Robotics, Graphics and Computer Vision*

# Overview

In this last exercise, you will combine everything you’ve learned to write integrators for full light transport. After completing this assignment, you will be able to render global illumination, caustics, depth of field and more with your code.

The integrators you will implement in this assignment compute solutions to the light transport integral

$$L_o(\mathbf{x},\omega_o) = L_e(\mathbf{x},\omega_o) + \int_\Omega f_r(\mathbf{x},\omega_o,\omega_i)\,L_i(\mathbf{x},\omega_i)\cos\theta_i\,d\omega_i \qquad (1)$$

where *fr* is the BRDF, **x** is a surface position and *Li* and *Lo* denote the incident and outgoing radiance functions. Unlike the previous assignment, we do not truncate the recursion - we won’t just compute direct lighting, but the full solution of this equation.

**Note:** This assignment builds heavily on code from Assignment 3. Make sure your solution to Assignment 3 works correctly before proceeding to work on this assignment.

# 1 Path Tracing (90%)

## 1.1 Naive path tracing (30%)

In this task, you will implement a naive path tracer to compute global illumination. This integrator will be very similar to the BSDF integrator for direct illumination you implemented in the previous assignment using material-based sampling (i.e. direct\_mats.cpp).

Create a path.cpp file, and create a new integrator called PathTracing. Use the NORI\_REGISTER\_CLASS to register the new integrator as "path" as: NORI\_REGISTER\_CLASS(PathTracing, "path");

Your integrator should proceed similar to the BSDF integrator from the previous assignment: Trace a ray from the camera, sample the BSDF and trace a ray in the sampled direction. However, unlike the previous assignment, your code should repeat this process - at the hitpoint of the second ray, your code should again sample a direction and trace a new ray, and repeat until the path leaves the scene. You should use the Russian roulette technique presented in the lecture as a termination criterion to avoid infinite recursion.

In order to test this first part, you can use the scene in assignment-4/cbox/cbox\_path.xml with only diffuse objects. It should converge to the reference image in scenes/assignment-4/references.

## 1.2 Path tracing with Next-Event Estimation (30%)

The integrator implemented in the previous subtask is a great start, but for scenes with small light sources it can perform poorly. For this reason, implement a new integrator that performs next event estimation. Create a path\_nee.cpp file, and create a new integrator called PathTracingNEE. Use the NORI\_REGISTER\_CLASS to register the new integrator as "path\_nee" as:

NORI\_REGISTER\_CLASS(PathTracingNEE, "path\_nee");

Your integrator will now generate two rays at every bounce: a recursive ray sampled from the BSDF as before, and one ray produced using emitter sampling. That is, at each bounce your code should accumulate radiance using your emitter sampling code from the previous assignment, and then generate the next point on the path using BSDF sampling. Use your implementation of direct illumination based on light sampling in direct\_ems.cpp as a guide for implementing NEE. To avoid double counting, do not accumulate radiance from light sources that were hit using BSDF sampling, except if the BSDF is perfectly smooth. This is because perfectly smooth BSDFs cannot be sampled using emitter sampling.

You can check if a BSDF is perfectly smooth by sampling a direction and checking if the measure field of the BSDFQueryRecord is EDiscrete.

Implementing a path tracer with NEE iteratively might be difficult, especially when dealing with directly visible light sources. We encourage you to implement it recursively to account for the first bounce appropriately.

For debugging, use the scene in assignment-4/cbox/cbox\_nee.xml and compare it against assignment-4/cbox/cbox\_path.xml. They should converge to the same result.

## 1.3 Path tracing with MIS (30%)

The integrator implemented in the previous subtask works great dealing with direct illumination, and will perform nicely in many scenes. However, for very specular surfaces or low-frequency illumination (e.g. large area lights) might introduce significant variance. For this reason, you would like to combine the estimate of BRDF and light sampling using Multiple Importance Sampling. Create a path\_mis.cpp file, and create a new integrator called PathTracingMIS. Use the NORI\_REGISTER\_CLASS to register the new integrator as "path\_mis" as:

NORI\_REGISTER\_CLASS(PathTracingMIS, "path\_mis");

Similar to the previous integrator, PathTracingMIS will now generate two rays at every bounce: a recursive ray sampled from the BSDF as before, and one ray produced using emitter sampling. However, at each bounce now you should accumulate radiance using your emitter sampling code from the previous assignment, and weight it using multiple importance sampling. Then, generate the next point on the path using BSDF sampling; if the next hitpoint is on a light source, accumulate its radiance, weighted by the MIS weight for BSDF sampling.

For debugging, use the scene in assignment-4/cbox/cbox\_mis.xml and compare it against assignment-4/cbox/cbox\_path.xml. They should converge to the same result.

**Note:** Each subtask is needed to complete the following one (i.e. for a path tracer with NEE, you will need to implement a path tracer first, and for accounting for MIS in NEE, you’ll need to have implemented NEE. Reuse as much code as needed, but be aware that a bug in a simpler version of the code will propagate to the rest. Additionally, note that NEE is in essence combining direct light sampling with recursive estimate of the BRDF sampling. Therefore, you can reuse code from the previous assignment.

# 2 Interesting Scene (10%)

Make your own interesting scene by modifying the XML files to change the materials, load new OBJ files or move existing objects, change existing light sources or add new ones, etc. Render the scene using any combination of the implemented techniques (naive path tracing, NEE, MIS). Name the resulting renders as interesting\_X\_INT.exr and interesting\_X\_INT.png, where X is the scene index in case you rendered more than one scene, and INT accounts for the integrator used—i.e. path, nee or mis.

# 3 References

You may find the following general references useful:

• *"Physically Based Rendering, Fourth Edition: From Theory To Implementation"* by Matt Pharr, Wenzel Jakob, and Greg Humphreys. The MIT Press, 4th edition, March 2023. [http://www.pbr-book.org/](<http://www.pbr-book.org/>)

• *"Global Illumination Compendium - The Concise Guide to Global Illumination Algorithms",* Philip Dutre, 2003. [https://people.cs.kuleuven.be/philip.dutre/GI/](<https://people.cs.kuleuven.be/philip.dutre/GI/>)

• *"Robust Monte Carlo Methods for Light Transport Simulation",* PhD Thesis by Eric Veach, Stanford University, December 1997. [https://graphics.stanford.edu/papers/veach\_thesis/thesis-bw.pdf](<https://graphics.stanford.edu/papers/veach_thesis/thesis-bw.pdf>)

• Online Nori docs: [https://www.cs.cornell.edu/courses/cs6630/2012sp/nori/api/annotated.html/](<https://www.cs.cornell.edu/courses/cs6630/2012sp/nori/api/annotated.html>)

Feel free to consult additional references when completing projects, but remember to cite them in your writeup. When asked to implement feature X, we request that you don’t go and read the source code of the implementation of X in some other renderer, because you will likely not learn much in the process. The PBRT book is excluded from this rule. If in doubt, get in touch with the course staff.

# 4 What to submit?

For Section 1, test your implementation of the three integrators in the scenes provided in the scenes patch in Moodle, including /cbox/cbox\_INT.xml, /cbox/cbox\_glossy\_INT.xml, /cbox/cbox\_caustics\_INT.xml, /veach\_mi/veach\_INT.xml, and /table/table\_INT.xml, where INT accounts for path, nee and mis depending on the integrator used). For Section 2 test your interesting scene using the implemented integrator (or integrators) of your choice that better display the features you consider relevant, and comment them on the README file. Note that this time there is no code patched for the assignment.

You should submit all your rendered figures and the source code you generated in this assignment. The submission should be a zip file with name p4\_NIP1\_NIP2.zip, and with the following structure:

• A file README.txt including the name of the authors (as sanity check) and all references consulted during the development of the assignment. Also, include a brief one-line comment about the features shown in the scene submitted for Section 2. Finally, this file should also contain a disclaimer about your usage of generative AI for the code, in which you need to explain in around two paragraphs which coding tasks have been done by generative AI, why have you needed generative AI for each coding task and which AI model have you used.

• A folder ./figures including all generated figures (both exr and png); the figures should be named with the name of the xml scene, appended with the number of samples used to generate these figures.

• A folder with all source files you modified or added in your implementation.

In case your file gets too big for the submission system, you can just include a link to a shared folder (e.g. Google Drive) with additional figures in README.txt.

The **deadline** for this task will be just before the first session of the final project corresponding to the lab group you are enrolled on.
