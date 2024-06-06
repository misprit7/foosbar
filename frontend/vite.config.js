export default {
    root: './',
    publicDir: './assets/',
    server: {
        port: 8000,
        host: '0.0.0.0',
        open: !(
            'SANDBOX_URL' in process.env || 'CODESANDBOX_HOST' in process.env
        ),
    },
    build: {
        outDir: './dist',
        emptyOutDir: true,
        sourcemap: true,
    },
};
