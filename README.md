# GLC(Generl Linear Camera) Renderer

#### Based on the paper : [http://csbio.unc.edu/mcmillan/pubs/eccv2004_Yu.pdf]

##### Using techniques:

1. GLC model generation (realized 3 types of camera models: perspective, orthogonal, pushbroom)
2. Ray Generation(GLC method) & Ray Casting: from camera model plane to objects(.obj file input)
3. Ray-Object Interaction
4. Rasterization (base on normal) 

##### render effect images:

###### cube in perspective projection
![alt text](glc_4.png "1")

###### cube in orthogonal projection
![alt text](glc_2.png "2")

###### cube in pushbroom projection
![alt text](glc_3.png "3")

###### 
![alt text](glc_5.png "4")

###### 3D splashing with phong lighting
![alt text](glc_6.png "5")
