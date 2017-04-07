NexusCoder

	float error
	int coord_q;
	int norm_q;
	int coord_q;
	int uv_q;

NexusEncoder(



NexusCoder(nvert, nface);

//if specified use the grid, else an euristic on length of edges.
//if offset specified use to define the grid. else use quantized minimum (to avoid problems with very out of origin.
coder.addCoords(float *buffer, float quantization = 0, Point3f offset = ); //3x
//recompute the grid to fit the model
setCoordBits(int bits);

coder.addColors(uchar *buffer, int luma_bits, int croma_bits, int alpha_bits); //4x
coder.addNormals(float *buffer, int bits); //3x
coder.addNormals(int16_t *buffer, int bits);
coder.addTex(float *buffer, int bits); //2x
coder.addData(float *buffer, int bits); //1x

coder.addIndex(uint32_t *buff);
coder.addIndex(uint16_t *buff);
coder.addGroups(int len, uint32_t *buffer); //30, 120, 255; //split into groups the index 0-29, 30-119, in TRIANGLES, last should nvert.
//external mechanism for groups passing

decoder.encode();



NexusDecoder decoder(len, uchar *buffer); //decodes first part of the mesh
int nvert;
int nface;
bool hasindex, normals etc.

decoder.setCoordsBuffer(float *buffer); //nvert*3 expected
decoder.setColorsBuffer(uchar *buffer);
decoder.setNormalsBuffer(float *buffer);
decoder.setNormalsBuffer(int16_t *buffer);
decoder.setTexBuffer(float *buffer);
decoder.setDataBuffer(float *buffer);

float *coords();

decoder.setIndex(uint32_t *buffer);
decoder.setIndex(uint16_t *buffer);


decoder.decode();

float *getCoords(); //in case setcoords not enabled
float *getNormals();





