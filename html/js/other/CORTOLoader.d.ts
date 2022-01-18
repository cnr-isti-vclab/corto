import {
	Loader,
	LoadingManager,
	BufferGeometry
} from '../../../src/Three';

export class CORTOLoader extends Loader {

	constructor( manager?: LoadingManager );

	load( url: string, onLoad: ( geometry: BufferGeometry ) => void, onProgress?: ( event: ProgressEvent ) => void, onError?: ( event: ErrorEvent ) => void ): void;
	setDecoderPath( path: string ): CORTOLoader;
	setShortIndex( enabled: number ): CORTOLoader;
	setShortNormals( enabled: number ): CORTOLoader;
	setWorkerLimit( workerLimit: number ): CORTOLoader;
	dispose(): CORTOLoader;
}
