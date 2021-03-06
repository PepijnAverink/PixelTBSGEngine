#pragma once

#define THREADSAFEWORLD 1
#define SQR(x) ((x)*(x))

namespace Tmpl8
{

	struct TransparentInfo
	{
	public:
		int   ID;
		float Transparency;
	};

struct BrickInfo { uint zeroes; /* , location; */ };

// Sprite system overview:
// The world contains a set of 0 or more sprites, typically loaded from .vox files.
// Sprites act like classic homecomputer sprites: they do not affect the world in
// any way, they just get displayed. Internally, this works as follows:
// 1. Before Game::Tick is executed:
//    - the world gets rendered by the GPU
//    - the sprites are then removed from the world
// 2. Game::Tick is now executed on a world that does not contain the sprites.
// 3. After Game::Tick completes:
//    - each sprite makes a backup of the voxels it overlaps
//    - the sprites are added back to the world
//    - the world is synchronized with the GPU for rendering in step 1.
// Consequence of this system is that even stationary sprites take time to process.

class SpriteFrame
{
public:
	~SpriteFrame() { delete buffer; }
	uchar* buffer = 0;
	int3 size = make_int3( 0 );			// size of the sprite over x, y and z
};

class Sprite
{
public:
	vector<SpriteFrame*> frame;				// sprite frames
	SpriteFrame* backup;					// backup of pixels that the sprite overwrote
	int3 lastPos		= make_int3( -9999 );	// location where the backup will be restored to
	int3 currPos		= make_int3( -9999 );	// location where the sprite will be drawn
	int currFrame		= 0;					// frame to draw
	uint3 scale			= { 1, 1, 1 };
	float4 rotation     = { 0.0f, 0.0f, 0.0f, 0.0f };
	float4 lastRotation = { 0.0f, 0.0f, 0.0f, 0.0f };

	int3 pivot = make_int3(0, 0, 0);
	uchar alpha = 7; // Set 3 least significant bits
	uchar backupAlpha = 0;
};

class Particle
{
public:
	int3 lastPos = make_int3(-9999);
	int3 currPos = make_int3(-9999);

	//red is (0 - 7) << 5, green is (0 - 7) << 2, blue (0 - 3)
	uchar color = 0;
	uchar alpha = 7;
	bool active = true;
};

class ParticleSystem
{
public:
	virtual ~ParticleSystem() 
	{
		for (uint i = 0; i < Particle.size(); i++)
			delete Particle[i];
	}

	virtual void Update(float dt) = 0;

	int3 currPos = make_int3(-9999);

	std::vector<Particle*> Particle;
	bool Active = true;
};

// Tile system overview:
// The top-level grid / brick layout of the world (see below) fits well with the 
// classic concept of tiled graphics. A tile is simply an 8x8x8 or 16x16x16 chunk of 
// voxel data, which can be placed in the world at locations that are a multiple of 8 
// over x, y and z. Drawing a tile will thus simply overwrite the contents of a brick 
// (or 8 bricks, when using the larger 16x16x16 tiles).

class Tile
{
public:
	Tile() = default;
	Tile( const char* voxFile );
	uchar voxels[BRICKSIZE * 2];		// tile voxel data
	uint zeroes;						// number of transparent voxels in the tile
};

class BigTile
{
public:
	BigTile() = default;
	BigTile( const char* voxFile );
	Tile tile[8];						// a big tile is just 2x2x2 tiles stored together
};

// Voxel world data structure:
// The world consists of a 128x128x128 top-level grid. Each cell in this grid can
// either store a solid color, or the index of an 8x8x8 brick. Filling all cells with 
// brick indices yields the maximum world resolution of 1024x1024x01024.
// Voxels are 8-bit values. '0' is an empy voxel; all other colors are opaque. Voxel
// colors are 3-3-2 rgb values. Note that black voxels do not exist in this scheme.
// The data structure is mirrored to the GPU, with a delay of 1 frame (i.e., the GPU
// always renders the previous frame). 
// Furthermore, since the full dataset is a bit over 1GB, only changes are synced.
// the CPU to GPU communication consists of 8MB for the 128x128x128 top-level ints,
// plus up to 8192 changed bricks. If more changes are made per frame, these will
// be postponed to the next frame.

class World
{
public:
	// constructor / destructor
	World( const uint targetID0);
	~World();
	// initialization
	void Clear();
	void DummyWorld();
	void LoadSky( const char* filename, const char* bin_name );
	// camera
	void SetCameraMatrix( const mat4& m ) { camMat = m; }
	float3 GetCameraViewDir() { return make_float3( camMat[2], camMat[6], camMat[10] ); }
	mat4& GetCameraMatrix() { return camMat; }
	// render flow
	void Commit();
	void Render();
	// high-level voxel access
	void Sphere( const float x, const float y, const float z, const float r, const uint c );
	void HDisc( const float x, const float y, const float z, const float r, const uint c );
	void Print( const char* text, const uint x, const uint y, const uint z, const uint c );
	uint LoadSprite( const char* voxFile );
	uint CloneSprite( const uint idx );
	uint SpriteFrameCount( const uint idx );
	void MoveSpriteTo( const uint idx, const uint x, const uint y, const uint z );
	void SetSpriteFrame( const uint idx, const uint frame );
	void ScaleSprite(const uint idx, const uint3 scale);
	void RotateSprite(const uint idx, const float x, const float y, const float z, const float a);
	void SetSpritePivot(const uint idx, const int x, const int y, const int z);
	void SetSpriteAlpha(const uint idx, const int a);
	void EnableSprite(const uint idx);
	void DisableSprite(const uint idx);
	void DestroySprite(const uint idx);
	bool IsSpriteDisabled(const uint idx);
	void SpawnParticleSystem(ParticleSystem* system);
	void UpdateParticleSystems(float dt);
	uint RayCast(const float3 origin, const float3 direction);
	uint LoadTile( const char* voxFile );
	uint LoadBigTile( const char* voxFile );
	void DrawTile( const uint idx, const uint x, const uint y, const uint z );
	void DrawTiles( const char* tileString, const uint x, const uint y, const uint z );
	void DrawBigTile( const uint idx, const uint x, const uint y, const uint z );
	void DrawBigTiles( const char* tileString, const uint x, const uint y, const uint z );

	void LoadTerainFromSprite(const uint idx, const uint x, const uint y, const uint z);
	void LoadTerainFromBigTile(const uint idx, const uint x, const uint y, const uint z);
	void DestroyTerrain(const int idx, const uint x, const uint y, const uint z);
	// inline ray tracing / cpu-only ray tracing
	uint TraceRay( float4 A, const float4 B, float& dist, float3& N, int steps );
	void TracePacket( float3 O, const float3 P1, const float3 P2, const float3 P3, const float3 P4 );
	// block scrolling
	void ScrollX( const int offset );
	void ScrollY( const int offset );
	void ScrollZ( const int offset );
private:
	// internal methods
	void RemoveSprite( const uint idx );
	void DrawSprite( const uint idx );
	void RemoveParticle(ParticleSystem* _system);
	void DrawParticles(ParticleSystem* _system);

	void DrawTileVoxels( const uint cellIdx, const uchar* voxels, const uint zeroes );
public:
	// low-level voxel access
	__forceinline uint Get( const uint x, const uint y, const uint z )
	{
		// calculate brick location in top-level grid
		const uint bx = (x / BRICKDIM) & (GRIDWIDTH - 1);
		const uint by = (y / BRICKDIM) & (GRIDHEIGHT - 1);
		const uint bz = (z / BRICKDIM) & (GRIDDEPTH - 1);
		const uint cellIdx = bx + bz * GRIDWIDTH + by * GRIDWIDTH * GRIDDEPTH;
		const uint g = grid[cellIdx];
		if ((g & 1) == 0 /* this is currently a 'solid' grid cell */) return g >> 1;

		const uint firIdx = ((g >> 1) / uint(BRICKS_PER_BUFFER));
		const uint modIdx = ((g >> 1) % uint(BRICKS_PER_BUFFER));
		// calculate the position of the voxel inside the brick
		const uint lx = x & (BRICKDIM - 1), ly = y & (BRICKDIM - 1), lz = z & (BRICKDIM - 1);
		return bricks[firIdx][(modIdx * BRICKSIZE + lx + ly * BRICKDIM + lz * BRICKDIM * BRICKDIM) * 2];
	}
	__forceinline uint GetM(const uint x, const uint y, const uint z)
	{
		// calculate brick location in top-level grid
		const uint bx = (x / BRICKDIM) & (GRIDWIDTH - 1);
		const uint by = (y / BRICKDIM) & (GRIDHEIGHT - 1);
		const uint bz = (z / BRICKDIM) & (GRIDDEPTH - 1);
		const uint cellIdx = bx + bz * GRIDWIDTH + by * GRIDWIDTH * GRIDDEPTH;
		const uint g = grid[cellIdx];
		if ((g & 1) == 0 /* this is currently a 'solid' grid cell */) return g >> 1;

		const uint firIdx = ((g >> 1) / uint(BRICKS_PER_BUFFER));
		const uint modIdx = ((g >> 1) % uint(BRICKS_PER_BUFFER));
		// calculate the position of the voxel inside the brick
		const uint lx = x & (BRICKDIM - 1), ly = y & (BRICKDIM - 1), lz = z & (BRICKDIM - 1);
		return bricks[firIdx][(modIdx * BRICKSIZE + lx + ly * BRICKDIM + lz * BRICKDIM * BRICKDIM) * 2 + 1];
	}

	__forceinline void Set( const uint x, const uint y, const uint z, const uint v /* actually an 8-bit value */, const uchar a = 7 )
	{
		// calculate brick location in top-level grid
		const uint bx = x / BRICKDIM & (GRIDWIDTH - 1);
		const uint by = y / BRICKDIM & (GRIDHEIGHT - 1);
		const uint bz = z / BRICKDIM & (GRIDDEPTH - 1);
		const uint cellIdx = bx + bz * GRIDWIDTH + by * GRIDWIDTH * GRIDDEPTH;
		// obtain current brick identifier from top-level grid
		uint g = grid[cellIdx], g1 = g >> 1;
		if ((g & 1) == 0 /* this is currently a 'solid' grid cell */)
		{
			if (g1 == v) return; // about to set the same value; we're done here
			const uint newIdx = NewBrick();
			const uint firIdx = (newIdx / uint(BRICKS_PER_BUFFER));
			const uint modIdx = (newIdx % uint(BRICKS_PER_BUFFER));

		#if BRICKDIM == 8
			// fully unrolled loop for writing the 512 bytes needed for a single brick, faster than memset
			const __m256i zero8 = _mm256_set1_epi8( static_cast<char>(g1) );
			__m256i* d8 = (__m256i*)(bricks[firIdx] + modIdx * 2 * BRICKSIZE);
			d8[0]  = zero8, d8[1]  = zero8, d8[2]  = zero8, d8[3]  = zero8;
			d8[4]  = zero8, d8[5]  = zero8, d8[6]  = zero8, d8[7]  = zero8;
			d8[8]  = zero8, d8[9]  = zero8, d8[10] = zero8, d8[11] = zero8;
			d8[12] = zero8, d8[13] = zero8, d8[14] = zero8, d8[15] = zero8;

			d8[16] = zero8, d8[17] = zero8, d8[18] = zero8, d8[19] = zero8;
			d8[20] = zero8, d8[21] = zero8, d8[22] = zero8, d8[23] = zero8;
			d8[24] = zero8, d8[25] = zero8, d8[26] = zero8, d8[27] = zero8;
			d8[28] = zero8, d8[29] = zero8, d8[30] = zero8, d8[31] = zero8;
		#else
			// let's keep the memset in case we want to experiment with other brick sizes
			memset( brick + newIdx * BRICKSIZE, g1, BRICKSIZE ); // copy solid value to brick
		#endif
			// we keep track of the number of zeroes, so we can remove fully zeroed bricks
			brickInfo[newIdx].zeroes = g == 0 ? BRICKSIZE : 0;
			g1 = newIdx, grid[cellIdx] = g = (newIdx << 1) | 1;
			// brickInfo[newIdx].location = cellIdx; // not used yet
		}

		const uint firIdx = (g1 / uint(BRICKS_PER_BUFFER));
		const uint modIdx = (g1 % uint(BRICKS_PER_BUFFER));

		// calculate the position of the voxel inside the brick
		const uint lx = x & (BRICKDIM - 1), ly = y & (BRICKDIM - 1), lz = z & (BRICKDIM - 1);
		const uint voxelIdx = (modIdx * BRICKSIZE + lx + ly * BRICKDIM + lz * BRICKDIM * BRICKDIM) * 2;
		const uint cv = bricks[firIdx][voxelIdx];
		if ((brickInfo[g1].zeroes += (cv != 0 && v == 0) - (cv == 0 && v != 0)) < BRICKSIZE)
		{
			bricks[firIdx][voxelIdx] = v;
			bricks[firIdx][voxelIdx + 1] = a;
			Mark( g1 ); // tag to be synced with GPU
			return;
		}
		grid[cellIdx] = 0;	// brick just became completely zeroed; recycle
		UnMark( g1 );		// no need to send it to GPU anymore
		FreeBrick( g1 );
	}
private:
	uint NewBrick()
	{
	#if THREADSAFEWORLD
		// get a fresh brick from the circular list in a thread-safe manner and without false sharing
		const uint trashItem = InterlockedAdd( &trashTail, 31 ) - 31;
		return trash[trashItem & (BRICKCOUNT - 1)];
	#else
		// slightly faster to not prevent false sharing if we're doing single core updates only
		return trash[trashTail++ & (BRICKCOUNT - 1)];
	#endif
	}
	void FreeBrick( const uint idx )
	{
		#if THREADSAFEWORLD
		// thread-safe access of the circular list
		const uint trashItem = InterlockedAdd( &trashHead, 31 ) - 31;
		trash[trashItem & (BRICKCOUNT - 1)] = idx;
	#else
		// for single-threaded code, a stepsize of 1 maximizes cache coherence.
		trash[trashHead++ & (BRICKCOUNT - 1)] = idx;
	#endif
	}
	void CheckBrick( const uint idx );
	void Mark( const uint idx )
	{
	#if THREADSAFEWORLD
		// be careful, setting a bit in an array is not thread-safe without _interlockedbittestandset
		_interlockedbittestandset( (LONG*)modified + (idx >> 5), idx & 31 );
	#else
		modified[idx >> 5] |= 1 << (idx & 31);
	#endif
	}
	void UnMark( const uint idx )
	{
	#if THREADSAFEWORLD
		// be careful, resetting a bit in an array is not thread-safe without _interlockedbittestandreset
		_interlockedbittestandreset( (LONG*)modified + (idx >> 5), idx & 31 );
	#else
		modified[idx >> 5] &= 0xffffffffu - (1 << (idx & 31));
	#endif
	}
	bool IsDirty( const uint idx ) { return (modified[idx >> 5] & (1 << (idx & 31))) > 0; }
	bool IsDirty32( const uint idx ) { return modified[idx] != 0; }
	void ClearMarks32( const uint idx ) { modified[idx] = 0; }
	void ClearMarks() { memset( modified, 0, (BRICKCOUNT / 32) * 4 ); }
	// helpers
	static void StreamCopy( __m256i* dst, const __m256i* src, const uint bytes )
	{
		// https://stackoverflow.com/questions/2963898/faster-alternative-to-memcpy
		assert( (bytes & 31) == 0 );
		uint N = bytes / 32;
		for (; N > 0; N--, src++, dst++)
		{
			const __m256i d = _mm256_stream_load_si256( src );
			_mm256_stream_si256( dst, d );
		}
	}
	void StreamCopyMT( __m256i* dst, __m256i* src, const uint bytes );
	// helper class for multithreaded memcpy
	class CopyJob : public Job
	{
	public:
		void Main() { World::StreamCopy( dst, src, N * 32 ); }
		__m256i* dst, * src;
		uint N;
	};
	// data members
	mat4 camMat;						// camera matrix to be used for rendering
	uint* grid = 0;						// pointer to host-side copy of the top-level grid
	Buffer* brickBuffer;
	Buffer* brickMaterialBuffer;

	Buffer* irradianceTraceBuffer;
	Buffer* normalTraceBuffer;
	Buffer* depthTraceBuffer;
	Kernel* traceProbeKernel;

	Buffer* giPathTraceResultBuffer;

	cl_mem  irradianceProbeTexture;
	Buffer* irradianceProbeBuffer;
	Buffer* depthProbeBuffer;

	cl_mem  depthProbeTexture;
	Kernel* updateProbeKernel;

	Kernel* testKernel;

	uchar* bricks[2];					// pointer to host-side copy of the bricks
//	uchar* brickMaterial = 0;
	uint* modified = 0;					// bitfield to mark bricks for synchronization
	BrickInfo* brickInfo = 0;			// maintenance data for bricks: zeroes, location
	volatile inline static LONG trashHead = BRICKCOUNT;	// thrash circular buffer tail
	volatile inline static LONG trashTail = 0;	// thrash circular buffer tail
	uint* trash = 0;					// indices of recycled bricks
	Buffer* screen = 0;					// OpenCL buffer that encapsulates the target OpenGL texture
	uint targetTextureID = 0;			// OpenGL render target
	Buffer* prevFrame[2];				// storage for the previous frame, for TAA
	int prevFrameIdx = 0;				// index of the previous frame buffer that will be used for TAA
	Buffer* paramBuffer;				// OpenCL buffer that stores renderer parameters
	Buffer* sky;						// OpenCL buffer for a HDR skydome
	Buffer* blueNoise;					// blue noise data
	int2 skySize;						// size of the skydome bitmap
	RenderParams params;				// CPU-side copy of the renderer parameters
	Kernel* renderer, * committer;		// render kernel and commit kernel
	Kernel* finalize;					// finalize kernel (TAA)

	Kernel* kernel;
	cl_event kernelDone;

	cl_event copyDone, commitDone;		// events for queue synchronization
	cl_event renderDone;				// event used for profiling
	uint tasks = 0;						// number of changed bricks, to be passed to commit kernel
	bool copyInFlight = false;			// flag for skipping async copy on first iteration
	bool commitInFlight = false;		// flag to make next commit wait for previous to complete
	cl_mem devmem = 0;					// device-side commit buffer
	cl_mem gridMap;						// host-side 3D image for top-level
	
	cl_mem diffuseOutput;
	cl_mem globalIlluminationOutput;

	Surface* font;						// bitmap font for print command
	bool firstFrame = true;				// for doing things in the first frame
public: // TODO: protected
	vector<Sprite*> sprite;				// list of loaded sprites
	vector<uint>	freelist;
	vector<Tile*> tile;					// list of loaded tiles
	vector<BigTile*> bigTile;			// list of loaded big tiles
	vector<ParticleSystem*> system;
};

} // namespace Tmpl8